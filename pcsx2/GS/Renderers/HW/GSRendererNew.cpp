/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021 PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"
#include "GSRendererNew.h"
#include "GS/GSGL.h"

GSRendererNew::GSRendererNew()
	: GSRendererHW()
{
	// Hope nothing requires too many draw calls.
	m_drawlist.reserve(2048);

	memset(&m_conf, 0, sizeof(m_conf));

	m_prim_overlap = PRIM_OVERLAP_UNKNOW;
	ResetStates();
}

void GSRendererNew::SetupIA(const float& sx, const float& sy)
{
	GL_PUSH("IA");

	if (m_userhacks_wildhack && !m_isPackedUV_HackFlag && PRIM->TME && PRIM->FST)
	{
		for (unsigned int i = 0; i < m_vertex.next; i++)
			m_vertex.buff[i].UV &= 0x3FEF3FEF;
	}
	const bool unscale_pt_ln = m_userHacks_enabled_unscale_ptln && (GetUpscaleMultiplier() != 1);
	const GSDevice::FeatureSupport features = g_gs_device->Features();

	switch (m_vt.m_primclass)
	{
		case GS_POINT_CLASS:
			if (unscale_pt_ln)
			{
				if (features.point_expand)
				{
					m_conf.vs.point_size = true;
				}
				else if (features.geometry_shader)
				{
					m_conf.gs.expand = true;
					m_conf.cb_vs.point_size = GSVector2(16.0f * sx, 16.0f * sy);
				}
			}

			m_conf.gs.topology = GSHWDrawConfig::GSTopology::Point;
			m_conf.topology = GSHWDrawConfig::Topology::Point;
			m_conf.indices_per_prim = 1;
			break;

		case GS_LINE_CLASS:
			if (unscale_pt_ln)
			{
				if (features.line_expand)
				{
					m_conf.line_expand = true;
				}
				else if (features.geometry_shader)
				{
					m_conf.gs.expand = true;
					m_conf.cb_vs.point_size = GSVector2(16.0f * sx, 16.0f * sy);
				}
			}

			m_conf.gs.topology = GSHWDrawConfig::GSTopology::Line;
			m_conf.topology = GSHWDrawConfig::Topology::Line;
			m_conf.indices_per_prim = 2;
			break;

		case GS_SPRITE_CLASS:
			// Heuristics: trade-off
			// Lines: GPU conversion => ofc, more GPU. And also more CPU due to extra shader validation stage.
			// Triangles: CPU conversion => ofc, more CPU ;) more bandwidth (72 bytes / sprite)
			//
			// Note: severals openGL operation does draw call under the wood like texture upload. So even if
			// you do 10 consecutive draw with the geometry shader, you will still pay extra validation if new
			// texture are uploaded. (game Shadow Hearts)
			//
			// Note2: Due to MultiThreaded driver, Nvidia suffers less of the previous issue. Still it isn't free
			// Shadow Heart is 90 fps (gs) vs 113 fps (no gs)
			//
			// Note3: Some GPUs (Happens on GT 750m, not on Intel 5200) don't properly divide by large floats (e.g. FLT_MAX/FLT_MAX == 0)
			// Lines2Sprites predivides by Q, avoiding this issue, so always use it if m_vt.m_accurate_stq

			// If the draw calls contains few primitives. Geometry Shader gain with be rather small versus
			// the extra validation cost of the extra stage.
			//
			// Note: keep Geometry Shader in the replayer to ease debug.
			if (g_gs_device->Features().geometry_shader && !m_vt.m_accurate_stq && m_vertex.next > 32) // <=> 16 sprites (based on Shadow Hearts)
			{
				m_conf.gs.expand = true;

				m_conf.topology = GSHWDrawConfig::Topology::Line;
				m_conf.indices_per_prim = 2;
			}
			else
			{
				Lines2Sprites();

				m_conf.topology = GSHWDrawConfig::Topology::Triangle;
				m_conf.indices_per_prim = 6;
			}
			m_conf.gs.topology = GSHWDrawConfig::GSTopology::Sprite;
			break;

		case GS_TRIANGLE_CLASS:
			m_conf.gs.topology = GSHWDrawConfig::GSTopology::Triangle;
			m_conf.topology = GSHWDrawConfig::Topology::Triangle;
			m_conf.indices_per_prim = 3;
			break;

		default:
			__assume(0);
	}

	m_conf.verts = m_vertex.buff;
	m_conf.nverts = m_vertex.next;
	m_conf.indices = m_index.buff;
	m_conf.nindices = m_index.tail;
}

void GSRendererNew::EmulateZbuffer()
{
	if (m_context->TEST.ZTE)
	{
		m_conf.depth.ztst = m_context->TEST.ZTST;
		// AA1: Z is not written on lines since coverage is always less than 0x80.
		m_conf.depth.zwe = (m_context->ZBUF.ZMSK || (PRIM->AA1 && m_vt.m_primclass == GS_LINE_CLASS)) ? 0 : 1;
	}
	else
	{
		m_conf.depth.ztst = ZTST_ALWAYS;
	}

	// On the real GS we appear to do clamping on the max z value the format allows.
	// Clamping is done after rasterization.
	const u32 max_z = 0xFFFFFFFF >> (GSLocalMemory::m_psm[m_context->ZBUF.PSM].fmt * 8);
	const bool clamp_z = (u32)(GSVector4i(m_vt.m_max.p).z) > max_z;

	m_conf.cb_vs.max_depth = GSVector2i(0xFFFFFFFF);
	//ps_cb.MaxDepth = GSVector4(0.0f, 0.0f, 0.0f, 1.0f);
	m_conf.ps.zclamp = 0;

	if (clamp_z)
	{
		if (m_vt.m_primclass == GS_SPRITE_CLASS || m_vt.m_primclass == GS_POINT_CLASS)
		{
			m_conf.cb_vs.max_depth = GSVector2i(max_z);
		}
		else if (!m_context->ZBUF.ZMSK)
		{
			m_conf.cb_ps.TA_MaxDepth_Af.z = static_cast<float>(max_z) * 0x1p-32f;
			m_conf.ps.zclamp = 1;
		}
	}

	const GSVertex* v = &m_vertex.buff[0];
	// Minor optimization of a corner case (it allow to better emulate some alpha test effects)
	if (m_conf.depth.ztst == ZTST_GEQUAL && m_vt.m_eq.z && v[0].XYZ.Z == max_z)
	{
		GL_DBG("Optimize Z test GEQUAL to ALWAYS (%s)", psm_str(m_context->ZBUF.PSM));
		m_conf.depth.ztst = ZTST_ALWAYS;
	}
}

void GSRendererNew::EmulateTextureShuffleAndFbmask()
{
	// Uncomment to disable texture shuffle emulation.
	// m_texture_shuffle = false;

	bool enable_fbmask_emulation = false;
	if (g_gs_device->Features().texture_barrier)
	{
		enable_fbmask_emulation = GSConfig.AccurateBlendingUnit != AccBlendLevel::Minimum;
	}
	else
	{
		// FBmask blend level selection.
		// We do this becaue:
		// 1. D3D sucks.
		// 2. FB copy is slow, especially on triangle primitives which is unplayable with some games.
		// 3. SW blending isn't implemented yet.
		switch (GSConfig.AccurateBlendingUnit)
		{
			case AccBlendLevel::Ultra:
			case AccBlendLevel::Full:
			case AccBlendLevel::High:
				// Fully enable Fbmask emulation like on opengl, note misses sw blending to work as opengl on some games (Genji).
				// Debug
				enable_fbmask_emulation = true;
				break;
			case AccBlendLevel::Medium:
				// Enable Fbmask emulation excluding triangle class because it is quite slow.
				// Exclude 0x80000000 because Genji needs sw blending, otherwise it breaks some effects.
				enable_fbmask_emulation = ((m_vt.m_primclass != GS_TRIANGLE_CLASS) && (m_context->FRAME.FBMSK != 0x80000000));
				break;
			case AccBlendLevel::Basic:
				// Enable Fbmask emulation excluding triangle class because it is quite slow.
				// Exclude 0x80000000 because Genji needs sw blending, otherwise it breaks some effects.
				// Also exclude fbmask emulation on texture shuffle just in case, it is probably safe tho.
				enable_fbmask_emulation = (!m_texture_shuffle && (m_vt.m_primclass != GS_TRIANGLE_CLASS) && (m_context->FRAME.FBMSK != 0x80000000));
				break;
			case AccBlendLevel::Minimum:
				break;
		}
	}

	if (m_texture_shuffle)
	{
		m_conf.ps.shuffle = 1;
		m_conf.ps.dfmt = 0;

		bool write_ba;
		bool read_ba;

		ConvertSpriteTextureShuffle(write_ba, read_ba);

		// If date is enabled you need to test the green channel instead of the
		// alpha channel. Only enable this code in DATE mode to reduce the number
		// of shader.
		m_conf.ps.write_rg = !write_ba && g_gs_device->Features().texture_barrier && m_context->TEST.DATE;

		m_conf.ps.read_ba = read_ba;

		// Please bang my head against the wall!
		// 1/ Reduce the frame mask to a 16 bit format
		const u32& m = m_context->FRAME.FBMSK;
		const u32 fbmask = ((m >> 3) & 0x1F) | ((m >> 6) & 0x3E0) | ((m >> 9) & 0x7C00) | ((m >> 16) & 0x8000);
		// FIXME GSVector will be nice here
		const u8 rg_mask = fbmask & 0xFF;
		const u8 ba_mask = (fbmask >> 8) & 0xFF;
		m_conf.colormask.wrgba = 0;

		// 2 Select the new mask (Please someone put SSE here)
		if (rg_mask != 0xFF)
		{
			if (write_ba)
			{
				GL_INS("Color shuffle %s => B", read_ba ? "B" : "R");
				m_conf.colormask.wb = 1;
			}
			else
			{
				GL_INS("Color shuffle %s => R", read_ba ? "B" : "R");
				m_conf.colormask.wr = 1;
			}
			if (rg_mask)
				m_conf.ps.fbmask = 1;
		}

		if (ba_mask != 0xFF)
		{
			if (write_ba)
			{
				GL_INS("Color shuffle %s => A", read_ba ? "A" : "G");
				m_conf.colormask.wa = 1;
			}
			else
			{
				GL_INS("Color shuffle %s => G", read_ba ? "A" : "G");
				m_conf.colormask.wg = 1;
			}
			if (ba_mask)
				m_conf.ps.fbmask = 1;
		}

		if (m_conf.ps.fbmask && enable_fbmask_emulation)
		{
			m_conf.cb_ps.FbMask.r = rg_mask;
			m_conf.cb_ps.FbMask.g = rg_mask;
			m_conf.cb_ps.FbMask.b = ba_mask;
			m_conf.cb_ps.FbMask.a = ba_mask;

			// No blending so hit unsafe path.
			if (!PRIM->ABE || !g_gs_device->Features().texture_barrier)
			{
				GL_INS("FBMASK Unsafe SW emulated fb_mask:%x on tex shuffle", fbmask);
				m_conf.require_one_barrier = true;
			}
			else
			{
				GL_INS("FBMASK SW emulated fb_mask:%x on tex shuffle", fbmask);
				m_conf.require_full_barrier = true;
			}
		}
		else
		{
			m_conf.ps.fbmask = 0;
		}
	}
	else
	{
		m_conf.ps.dfmt = GSLocalMemory::m_psm[m_context->FRAME.PSM].fmt;

		const GSVector4i fbmask_v = GSVector4i::load((int)m_context->FRAME.FBMSK);
		const int ff_fbmask = fbmask_v.eq8(GSVector4i::xffffffff()).mask();
		const int zero_fbmask = fbmask_v.eq8(GSVector4i::zero()).mask();

		m_conf.colormask.wrgba = ~ff_fbmask; // Enable channel if at least 1 bit is 0

		m_conf.ps.fbmask = enable_fbmask_emulation && (~ff_fbmask & ~zero_fbmask & 0xF);

		if (m_conf.ps.fbmask)
		{
			m_conf.cb_ps.FbMask = fbmask_v.u8to32();
			// Only alpha is special here, I think we can take a very unsafe shortcut
			// Alpha isn't blended on the GS but directly copyied into the RT.
			//
			// Behavior is clearly undefined however there is a high probability that
			// it will work. Masked bit will be constant and normally the same everywhere
			// RT/FS output/Cached value.
			//
			// Just to be sure let's add a new safe hack for unsafe access :)
			//
			// Here the GL spec quote to emphasize the unexpected behavior.
			/*
			   - If a texel has been written, then in order to safely read the result
			   a texel fetch must be in a subsequent Draw separated by the command

			   void TextureBarrier(void);

			   TextureBarrier() will guarantee that writes have completed and caches
			   have been invalidated before subsequent Draws are executed.
			 */
			// No blending so hit unsafe path.
			if (!PRIM->ABE || !(~ff_fbmask & ~zero_fbmask & 0x7) || !g_gs_device->Features().texture_barrier)
			{
				GL_INS("FBMASK Unsafe SW emulated fb_mask:%x on %d bits format", m_context->FRAME.FBMSK,
					(GSLocalMemory::m_psm[m_context->FRAME.PSM].fmt == 2) ? 16 : 32);
				m_conf.require_one_barrier = true;
			}
			else
			{
				// The safe and accurate path (but slow)
				GL_INS("FBMASK SW emulated fb_mask:%x on %d bits format", m_context->FRAME.FBMSK,
					(GSLocalMemory::m_psm[m_context->FRAME.PSM].fmt == 2) ? 16 : 32);
				m_conf.require_full_barrier = true;
			}
		}
	}
}

void GSRendererNew::EmulateChannelShuffle(GSTexture** rt, const GSTextureCache::Source* tex)
{
	// Uncomment to disable HLE emulation (allow to trace the draw call)
	// m_channel_shuffle = false;

	// First let's check we really have a channel shuffle effect
	if (m_channel_shuffle)
	{
		if (m_game.title == CRC::GT4 || m_game.title == CRC::GT3 || m_game.title == CRC::GTConcept || m_game.title == CRC::TouristTrophy)
		{
			GL_INS("Gran Turismo RGB Channel");
			m_conf.ps.channel = ChannelFetch_RGB;
			m_context->TEX0.TFX = TFX_DECAL;
			*rt = tex->m_from_target;
		}
		else if (m_game.title == CRC::Tekken5)
		{
			if (m_context->FRAME.FBW == 1)
			{
				// Used in stages: Secret Garden, Acid Rain, Moonlit Wilderness
				GL_INS("Tekken5 RGB Channel");
				m_conf.ps.channel = ChannelFetch_RGB;
				m_context->FRAME.FBMSK = 0xFF000000;
				// 12 pages: 2 calls by channel, 3 channels, 1 blit
				// Minus current draw call
				m_skip = 12 * (3 + 3 + 1) - 1;
				*rt = tex->m_from_target;
			}
			else
			{
				// Could skip model drawing if wrongly detected
				m_channel_shuffle = false;
			}
		}
		else if ((tex->m_texture->GetType() == GSTexture::Type::DepthStencil) && !(tex->m_32_bits_fmt))
		{
			// So far 2 games hit this code path. Urban Chaos and Tales of Abyss
			// UC: will copy depth to green channel
			// ToA: will copy depth to alpha channel
			if ((m_context->FRAME.FBMSK & 0xFF0000) == 0xFF0000)
			{
				// Green channel is masked
				GL_INS("Tales Of Abyss Crazyness (MSB 16b depth to Alpha)");
				m_conf.ps.tales_of_abyss_hle = 1;
			}
			else
			{
				GL_INS("Urban Chaos Crazyness (Green extraction)");
				m_conf.ps.urban_chaos_hle = 1;
			}
		}
		else if (m_index.tail <= 64 && m_context->CLAMP.WMT == 3)
		{
			// Blood will tell. I think it is channel effect too but again
			// implemented in a different way. I don't want to add more CRC stuff. So
			// let's disable channel when the signature is different
			//
			// Note: Tales Of Abyss and Tekken5 could hit this path too. Those games are
			// handled above.
			GL_INS("Maybe not a channel!");
			m_channel_shuffle = false;
		}
		else if (m_context->CLAMP.WMS == 3 && ((m_context->CLAMP.MAXU & 0x8) == 8))
		{
			// Read either blue or Alpha. Let's go for Blue ;)
			// MGS3/Kill Zone
			GL_INS("Blue channel");
			m_conf.ps.channel = ChannelFetch_BLUE;
		}
		else if (m_context->CLAMP.WMS == 3 && ((m_context->CLAMP.MINU & 0x8) == 0))
		{
			// Read either Red or Green. Let's check the V coordinate. 0-1 is likely top so
			// red. 2-3 is likely bottom so green (actually depends on texture base pointer offset)
			const bool green = PRIM->FST && (m_vertex.buff[0].V & 32);
			if (green && (m_context->FRAME.FBMSK & 0x00FFFFFF) == 0x00FFFFFF)
			{
				// Typically used in Terminator 3
				const int blue_mask = m_context->FRAME.FBMSK >> 24;
				int blue_shift = -1;

				// Note: potentially we could also check the value of the clut
				switch (blue_mask)
				{
					case 0xFF: ASSERT(0);      break;
					case 0xFE: blue_shift = 1; break;
					case 0xFC: blue_shift = 2; break;
					case 0xF8: blue_shift = 3; break;
					case 0xF0: blue_shift = 4; break;
					case 0xE0: blue_shift = 5; break;
					case 0xC0: blue_shift = 6; break;
					case 0x80: blue_shift = 7; break;
					default:                   break;
				}

				if (blue_shift >= 0)
				{
					const int green_mask = ~blue_mask & 0xFF;
					const int green_shift = 8 - blue_shift;

					GL_INS("Green/Blue channel (%d, %d)", blue_shift, green_shift);
					m_conf.cb_ps.ChannelShuffle = GSVector4i(blue_mask, blue_shift, green_mask, green_shift);
					m_conf.ps.channel = ChannelFetch_GXBY;
					m_context->FRAME.FBMSK = 0x00FFFFFF;
				}
				else
				{
					GL_INS("Green channel (wrong mask) (fbmask %x)", blue_mask);
					m_conf.ps.channel = ChannelFetch_GREEN;
				}
			}
			else if (green)
			{
				GL_INS("Green channel");
				m_conf.ps.channel = ChannelFetch_GREEN;
			}
			else
			{
				// Pop
				GL_INS("Red channel");
				m_conf.ps.channel = ChannelFetch_RED;
			}
		}
		else
		{
			GL_INS("Channel not supported");
			m_channel_shuffle = false;
		}
	}

	// Effect is really a channel shuffle effect so let's cheat a little
	if (m_channel_shuffle)
	{
		m_conf.raw_tex = tex->m_from_target;
		if (g_gs_device->Features().texture_barrier)
			m_conf.require_one_barrier = true;

		// Replace current draw with a fullscreen sprite
		//
		// Performance GPU note: it could be wise to reduce the size to
		// the rendered size of the framebuffer

		GSVertex* s = &m_vertex.buff[0];
		s[0].XYZ.X = (u16)(m_context->XYOFFSET.OFX + 0);
		s[1].XYZ.X = (u16)(m_context->XYOFFSET.OFX + 16384);
		s[0].XYZ.Y = (u16)(m_context->XYOFFSET.OFY + 0);
		s[1].XYZ.Y = (u16)(m_context->XYOFFSET.OFY + 16384);

		m_vertex.head = m_vertex.tail = m_vertex.next = 2;
		m_index.tail = 2;
	}
	else
	{
		m_conf.raw_tex = nullptr;
	}
}

void GSRendererNew::EmulateBlending(bool& DATE_PRIMID, bool& DATE_BARRIER)
{
	// AA1: Don't enable blending on AA1, not yet implemented on hardware mode,
	// it requires coverage sample so it's safer to turn it off instead.
	const bool AA1 = PRIM->AA1 && (m_vt.m_primclass == GS_LINE_CLASS);
	// PABE: Check condition early as an optimization.
	const bool PABE = PRIM->ABE && m_env.PABE.PABE && (GetAlphaMinMax().max < 128);

	// No blending or coverage anti-aliasing so early exit
	if (PABE || !(PRIM->ABE || AA1))
	{
		m_conf.blend = {};
		return;
	}

	// Compute the blending equation to detect special case
	const GIFRegALPHA& ALPHA = m_context->ALPHA;
	u8 blend_index = u8(((ALPHA.A * 3 + ALPHA.B) * 3 + ALPHA.C) * 3 + ALPHA.D);
	const int blend_flag = g_gs_device->GetBlendFlags(blend_index);

	// Do the multiplication in shader for blending accumulation: Cs*As + Cd or Cs*Af + Cd
	bool accumulation_blend = !!(blend_flag & BLEND_ACCU);

	// Blending doesn't require barrier, or sampling of the rt
	const bool blend_non_recursive = !!(blend_flag & BLEND_NO_REC);

	// BLEND MIX selection, use a mix of hw/sw blending
	const bool blend_mix1 = !!(blend_flag & BLEND_MIX1);
	const bool blend_mix2 = !!(blend_flag & BLEND_MIX2);
	const bool blend_mix3 = !!(blend_flag & BLEND_MIX3);
	bool blend_mix = (blend_mix1 || blend_mix2 || blend_mix3);

	// SW Blend is (nearly) free. Let's use it.
	const bool impossible_or_free_blend = (blend_flag & BLEND_A_MAX) // Impossible blending
		|| blend_non_recursive                 // Free sw blending, doesn't require barriers or reading fb
		|| accumulation_blend                  // Mix of hw/sw blending
		|| (m_prim_overlap == PRIM_OVERLAP_NO) // Blend can be done in a single draw
		|| (m_conf.require_full_barrier);      // Another effect (for example fbmask) already requires a full barrier

	// Warning no break on purpose
	// Note: the [[fallthrough]] attribute tell compilers not to complain about not having breaks.
	bool sw_blending = false;
	if (g_gs_device->Features().texture_barrier)
	{
		switch (GSConfig.AccurateBlendingUnit)
		{
			case AccBlendLevel::Ultra:
				sw_blending |= true;
				[[fallthrough]];
			case AccBlendLevel::Full:
				sw_blending |= ALPHA.A != ALPHA.B && ALPHA.C == 0 && GetAlphaMinMax().max > 128;
				[[fallthrough]];
			case AccBlendLevel::High:
				sw_blending |= ALPHA.C == 1 || (ALPHA.A != ALPHA.B && ALPHA.C == 2 && ALPHA.FIX > 128u);
				[[fallthrough]];
			case AccBlendLevel::Medium:
				// Initial idea was to enable accurate blending for sprite rendering to handle
				// correctly post-processing effect. Some games (ZoE) use tons of sprites as particles.
				// In order to keep it fast, let's limit it to smaller draw call.
				sw_blending |= m_vt.m_primclass == GS_SPRITE_CLASS && m_drawlist.size() < 100;
				[[fallthrough]];
			case AccBlendLevel::Basic:
				sw_blending |= impossible_or_free_blend;
				[[fallthrough]];
			case AccBlendLevel::Minimum:
				/*sw_blending |= accumulation_blend*/;
		}
	}
	else
	{
		if (static_cast<u8>(GSConfig.AccurateBlendingUnit) >= static_cast<u8>(AccBlendLevel::Basic))
			sw_blending |= accumulation_blend || blend_non_recursive;
	}

	// Do not run BLEND MIX if sw blending is already present, it's less accurate
	if (GSConfig.AccurateBlendingUnit != AccBlendLevel::Minimum)
	{
		blend_mix &= !sw_blending;
		sw_blending |= blend_mix;
	}

	// Color clip
	if (m_env.COLCLAMP.CLAMP == 0)
	{
		// Safe FBMASK, avoid hitting accumulation mode on 16bit,
		// fixes shadows in Superman shadows of Apokolips.
		const bool sw_fbmask_colclip = !m_conf.require_one_barrier && m_conf.ps.fbmask;
		bool free_colclip = false;
		if (g_gs_device->Features().texture_barrier)
			free_colclip = m_prim_overlap == PRIM_OVERLAP_NO || blend_non_recursive || sw_fbmask_colclip;
		else
			free_colclip = blend_non_recursive;
		GL_DBG("COLCLIP Info (Blending: %d/%d/%d/%d, SW FBMASK: %d, OVERLAP: %d)",
			ALPHA.A, ALPHA.B, ALPHA.C, ALPHA.D, sw_fbmask_colclip, m_prim_overlap);
		if (free_colclip)
		{
			// The fastest algo that requires a single pass
			GL_INS("COLCLIP Free mode ENABLED");
			m_conf.ps.colclip = 1;
			sw_blending = true;
			accumulation_blend = false; // disable the HDR algo
			blend_mix = false;
		}
		else if (accumulation_blend || blend_mix)
		{
			// A fast algo that requires 2 passes
			GL_INS("COLCLIP Fast HDR mode ENABLED");
			m_conf.ps.hdr = 1;
			sw_blending = true; // Enable sw blending for the HDR algo
		}
		else if (sw_blending && g_gs_device->Features().texture_barrier)
		{
			// A slow algo that could requires several passes (barely used)
			GL_INS("COLCLIP SW mode ENABLED");
			m_conf.ps.colclip = 1;
		}
		else
		{
			GL_INS("COLCLIP HDR mode ENABLED");
			m_conf.ps.hdr = 1;
		}
	}

	// Per pixel alpha blending
	if (m_env.PABE.PABE)
	{
		// Breath of Fire Dragon Quarter, Strawberry Shortcake, Super Robot Wars, Cartoon Network Racing.

		if (sw_blending)
		{
			GL_INS("PABE mode ENABLED");
			if (g_gs_device->Features().texture_barrier)
			{
				// Disable hw/sw blend and do pure sw blend with reading the framebuffer.
				accumulation_blend = false;
				blend_mix = false;
				m_conf.ps.pabe = 1;
			}
			else
			{
				m_conf.ps.pabe = blend_non_recursive;
			}
		}
		else if (ALPHA.A == 0 && ALPHA.B == 1 && ALPHA.C == 0 && ALPHA.D == 1)
		{
			// this works because with PABE alpha blending is on when alpha >= 0x80, but since the pixel shader
			// cannot output anything over 0x80 (== 1.0) blending with 0x80 or turning it off gives the same result
			blend_index = 0;
		}
	}

	// For stat to optimize accurate option
#if 0
	GL_INS("BLEND_INFO: %d/%d/%d/%d. Clamp:%d. Prim:%d number %d (drawlist %d) (sw %d)",
		ALPHA.A, ALPHA.B,  ALPHA.C, ALPHA.D, m_env.COLCLAMP.CLAMP, m_vt.m_primclass, m_vertex.next, m_drawlist.size(), sw_blending);
#endif
	if (sw_blending)
	{
		m_conf.ps.blend_a = ALPHA.A;
		m_conf.ps.blend_b = ALPHA.B;
		m_conf.ps.blend_c = ALPHA.C;
		m_conf.ps.blend_d = ALPHA.D;

		if (accumulation_blend)
		{
			// Keep HW blending to do the addition/subtraction
			m_conf.blend = {blend_index, 0, false, true, false};
			if (ALPHA.A == 2)
			{
				// The blend unit does a reverse subtraction so it means
				// the shader must output a positive value.
				// Replace 0 - Cs by Cs - 0
				m_conf.ps.blend_a = ALPHA.B;
				m_conf.ps.blend_b = 2;
			}
			// Remove the addition/substraction from the SW blending
			m_conf.ps.blend_d = 2;

			// Note accumulation_blend doesn't require a barrier
		}
		else if (blend_mix)
		{
			m_conf.blend = {blend_index, ALPHA.FIX, ALPHA.C == 2, false, true};
			m_conf.ps.alpha_clamp = 1;

			if (blend_mix1)
			{
				m_conf.ps.blend_a = 0;
				m_conf.ps.blend_b = 2;
				m_conf.ps.blend_d = 2;
			}
			else if (blend_mix2)
			{
				m_conf.ps.blend_a = 0;
				m_conf.ps.blend_b = 2;
				m_conf.ps.blend_d = 0;
			}
			else if (blend_mix3)
			{
				m_conf.ps.blend_a = 2;
				m_conf.ps.blend_b = 0;
				m_conf.ps.blend_d = 0;
			}
		}
		else
		{
			// Disable HW blending
			m_conf.blend = {};

			m_conf.require_full_barrier |= !blend_non_recursive;

			// Only BLEND_NO_REC should hit this code path for now
			if (!g_gs_device->Features().texture_barrier)
				ASSERT(blend_non_recursive);
		}

		// Require the fix alpha vlaue
		if (ALPHA.C == 2)
			m_conf.cb_ps.TA_MaxDepth_Af.a = static_cast<float>(ALPHA.FIX) / 128.0f;
	}
	else
	{
		m_conf.ps.clr1 = !!(blend_flag & BLEND_C_CLR);
		if (m_conf.ps.dfmt == 1 && ALPHA.C == 1)
		{
			// 24 bits doesn't have an alpha channel so use 1.0f fix factor as equivalent
			const u8 hacked_blend_index = blend_index + 3; // +3 <=> +1 on C
			m_conf.blend = {hacked_blend_index, 128, true, false, false};
		}
		else
		{
			m_conf.blend = {blend_index, ALPHA.FIX, ALPHA.C == 2, false, false};
		}
	}

	// DATE_PRIMID interact very badly with sw blending. DATE_PRIMID uses the primitiveID to find the primitive
	// that write the bad alpha value. Sw blending will force the draw to run primitive by primitive
	// (therefore primitiveID will be constant to 1).
	// Switch DATE_PRIMID with DATE_BARRIER in such cases to ensure accuracy.
	// No mix of COLCLIP + sw blend + DATE_PRIMID, neither sw fbmask + DATE_PRIMID.
	// Note: Do the swap in the end, saves the expensive draw splitting/barriers when mixed software blending is used.
	if (sw_blending && DATE_PRIMID && m_conf.require_full_barrier)
	{
		GL_PERF("DATE: Swap DATE_PRIMID with DATE_BARRIER");
		m_conf.require_full_barrier = true;
		DATE_PRIMID = false;
		DATE_BARRIER = true;
	}
}

void GSRendererNew::EmulateTextureSampler(const GSTextureCache::Source* tex)
{
	// Warning fetch the texture PSM format rather than the context format. The latter could have been corrected in the texture cache for depth.
	//const GSLocalMemory::psm_t &psm = GSLocalMemory::m_psm[m_context->TEX0.PSM];
	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[tex->m_TEX0.PSM];
	const GSLocalMemory::psm_t& cpsm = psm.pal > 0 ? GSLocalMemory::m_psm[m_context->TEX0.CPSM] : psm;

	const u8 wms = m_context->CLAMP.WMS;
	const u8 wmt = m_context->CLAMP.WMT;
	const bool complex_wms_wmt = !!((wms | wmt) & 2);

	const bool need_mipmap = IsMipMapDraw();
	const bool shader_emulated_sampler = tex->m_palette || cpsm.fmt != 0 || complex_wms_wmt || psm.depth;
	const bool trilinear_manual = need_mipmap && m_hw_mipmap == HWMipmapLevel::Full;

	bool bilinear = m_vt.IsLinear();
	int trilinear = 0;
	bool trilinear_auto = false;
	switch (GSConfig.UserHacks_TriFilter)
	{
		case TriFiltering::Forced:
			trilinear = static_cast<u8>(GS_MIN_FILTER::Linear_Mipmap_Linear);
			trilinear_auto = !need_mipmap || m_hw_mipmap != HWMipmapLevel::Full;
			break;

		case TriFiltering::PS2:
			if (need_mipmap && m_hw_mipmap != HWMipmapLevel::Full)
			{
				trilinear = m_context->TEX1.MMIN;
				trilinear_auto = true;
			}
			break;

		case TriFiltering::Off:
		default:
			break;
	}

	// 1 and 0 are equivalent
	m_conf.ps.wms = (wms & 2) ? wms : 0;
	m_conf.ps.wmt = (wmt & 2) ? wmt : 0;

	// Depth + bilinear filtering isn't done yet (And I'm not sure we need it anyway but a game will prove me wrong)
	// So of course, GTA set the linear mode, but sampling is done at texel center so it is equivalent to nearest sampling
	ASSERT(!(psm.depth && m_vt.IsLinear()));

	// Performance note:
	// 1/ Don't set 0 as it is the default value
	// 2/ Only keep aem when it is useful (avoid useless shader permutation)
	if (m_conf.ps.shuffle)
	{
		// Force a 32 bits access (normally shuffle is done on 16 bits)
		// m_ps_sel.tex_fmt = 0; // removed as an optimization
		m_conf.ps.aem = m_env.TEXA.AEM;
		ASSERT(tex->m_target);

		// Require a float conversion if the texure is a depth otherwise uses Integral scaling
		if (psm.depth)
		{
			m_conf.ps.depth_fmt = (tex->m_texture->GetType() != GSTexture::Type::DepthStencil) ? 3 : 1;
		}

		// Shuffle is a 16 bits format, so aem is always required
		GSVector4 ta(m_env.TEXA & GSVector4i::x000000ff());
		ta /= 255.0f;
		m_conf.cb_ps.TA_MaxDepth_Af.x = ta.x;
		m_conf.cb_ps.TA_MaxDepth_Af.y = ta.y;

		// The purpose of texture shuffle is to move color channel. Extra interpolation is likely a bad idea.
		bilinear &= m_vt.IsLinear();

		GSVector4 half_pixel = RealignTargetTextureCoordinate(tex);
		m_conf.cb_vs.texture_offset = GSVector2(half_pixel.x, half_pixel.y);
	}
	else if (tex->m_target)
	{
		// Use an old target. AEM and index aren't resolved it must be done
		// on the GPU

		// Select the 32/24/16 bits color (AEM)
		m_conf.ps.aem_fmt = cpsm.fmt;
		m_conf.ps.aem = m_env.TEXA.AEM;

		// Don't upload AEM if format is 32 bits
		if (cpsm.fmt)
		{
			GSVector4 ta(m_env.TEXA & GSVector4i::x000000ff());
			ta /= 255.0f;
			m_conf.cb_ps.TA_MaxDepth_Af.x = ta.x;
			m_conf.cb_ps.TA_MaxDepth_Af.y = ta.y;
		}

		// Select the index format
		if (tex->m_palette)
		{
			// FIXME Potentially improve fmt field in GSLocalMemory
			if (m_context->TEX0.PSM == PSM_PSMT4HL)
				m_conf.ps.pal_fmt = 1;
			else if (m_context->TEX0.PSM == PSM_PSMT4HH)
				m_conf.ps.pal_fmt = 2;
			else
				m_conf.ps.pal_fmt = 3;

			// Alpha channel of the RT is reinterpreted as an index. Star
			// Ocean 3 uses it to emulate a stencil buffer.  It is a very
			// bad idea to force bilinear filtering on it.
			bilinear &= m_vt.IsLinear();
		}

		// Depth format
		if (tex->m_texture->GetType() == GSTexture::Type::DepthStencil)
		{
			// Require a float conversion if the texure is a depth format
			m_conf.ps.depth_fmt = (psm.bpp == 16) ? 2 : 1;

			// Don't force interpolation on depth format
			bilinear &= m_vt.IsLinear();
		}
		else if (psm.depth)
		{
			// Use Integral scaling
			m_conf.ps.depth_fmt = 3;

			// Don't force interpolation on depth format
			bilinear &= m_vt.IsLinear();
		}

		GSVector4 half_pixel = RealignTargetTextureCoordinate(tex);
		m_conf.cb_vs.texture_offset = GSVector2(half_pixel.x, half_pixel.y);
	}
	else if (tex->m_palette)
	{
		// Use a standard 8 bits texture. AEM is already done on the CLUT
		// Therefore you only need to set the index
		// m_conf.ps.aem     = 0; // removed as an optimization

		// Note 4 bits indexes are converted to 8 bits
		m_conf.ps.pal_fmt = 3;
	}
	else
	{
		// Standard texture. Both index and AEM expansion were already done by the CPU.
		// m_conf.ps.tex_fmt = 0; // removed as an optimization
		// m_conf.ps.aem     = 0; // removed as an optimization
	}

	if (m_context->TEX0.TFX == TFX_MODULATE && m_vt.m_eq.rgba == 0xFFFF && m_vt.m_min.c.eq(GSVector4i(128)))
	{
		// Micro optimization that reduces GPU load (removes 5 instructions on the FS program)
		m_conf.ps.tfx = TFX_DECAL;
	}
	else
	{
		m_conf.ps.tfx = m_context->TEX0.TFX;
	}

	m_conf.ps.tcc = m_context->TEX0.TCC;

	m_conf.ps.ltf = bilinear && shader_emulated_sampler;
	m_conf.ps.point_sampler = g_gs_device->Features().broken_point_sampler && (!bilinear || shader_emulated_sampler);

	const int w = tex->m_texture->GetWidth();
	const int h = tex->m_texture->GetHeight();

	const int tw = (int)(1 << m_context->TEX0.TW);
	const int th = (int)(1 << m_context->TEX0.TH);

	const GSVector4 WH(tw, th, w, h);

	m_conf.ps.fst = !!PRIM->FST;

	m_conf.cb_ps.WH = WH;
	m_conf.cb_ps.HalfTexel = GSVector4(-0.5f, 0.5f).xxyy() / WH.zwzw();
	if (complex_wms_wmt)
	{
		m_conf.cb_ps.MskFix = GSVector4i(m_context->CLAMP.MINU, m_context->CLAMP.MINV, m_context->CLAMP.MAXU, m_context->CLAMP.MAXV);;
		m_conf.cb_ps.MinMax = GSVector4(m_conf.cb_ps.MskFix) / WH.xyxy();
	}
	else if (trilinear_manual)
	{
		// Reuse uv_min_max for mipmap parameter to avoid an extension of the UBO
		m_conf.cb_ps.MinMax.x = (float)m_context->TEX1.K / 16.0f;
		m_conf.cb_ps.MinMax.y = float(1 << m_context->TEX1.L);
		m_conf.cb_ps.MinMax.z = float(m_lod.x); // Offset because first layer is m_lod, dunno if we can do better
		m_conf.cb_ps.MinMax.w = float(m_lod.y);
	}
	else if (trilinear_auto)
	{
		tex->m_texture->GenerateMipmapsIfNeeded();
	}

	// TC Offset Hack
	m_conf.ps.tcoffsethack = m_userhacks_tcoffset;
	GSVector4 tc_oh_ts = GSVector4(1 / 16.0f, 1 / 16.0f, m_userhacks_tcoffset_x, m_userhacks_tcoffset_y) / WH.xyxy();
	m_conf.cb_ps.TCOffsetHack = GSVector2(tc_oh_ts.z, tc_oh_ts.w);
	m_conf.cb_vs.texture_scale = GSVector2(tc_oh_ts.x, tc_oh_ts.y);

	// Must be done after all coordinates math
	if (m_context->HasFixedTEX0() && !PRIM->FST)
	{
		m_conf.ps.invalid_tex0 = 1;
		// Use invalid size to denormalize ST coordinate
		m_conf.cb_ps.WH.x = (float)(1 << m_context->stack.TEX0.TW);
		m_conf.cb_ps.WH.y = (float)(1 << m_context->stack.TEX0.TH);

		// We can't handle m_target with invalid_tex0 atm due to upscaling
		ASSERT(!tex->m_target);
	}

	// Only enable clamping in CLAMP mode. REGION_CLAMP will be done manually in the shader
	m_conf.sampler.tau = (wms != CLAMP_CLAMP);
	m_conf.sampler.tav = (wmt != CLAMP_CLAMP);
	if (shader_emulated_sampler)
	{
		m_conf.sampler.biln = 0;
		m_conf.sampler.aniso = 0;
		m_conf.sampler.triln = 0;
	}
	else
	{
		m_conf.sampler.biln = bilinear;
		// Aniso filtering doesn't work with textureLod so use texture (automatic_lod) instead.
		// Enable aniso only for triangles. Sprites are flat so aniso is likely useless (it would save perf for others primitives).
		const bool anisotropic = m_vt.m_primclass == GS_TRIANGLE_CLASS && !trilinear_manual;
		m_conf.sampler.aniso = anisotropic;
		m_conf.sampler.triln = trilinear;
		if (trilinear_manual)
		{
			m_conf.ps.manual_lod = 1;
		}
		else if (trilinear_auto || anisotropic)
		{
			m_conf.ps.automatic_lod = 1;
		}
	}

	// clamp to base level if we're not providing or generating mipmaps
	// manual trilinear causes the chain to be uploaded, auto causes it to be generated
	m_conf.sampler.lodclamp = !(trilinear_manual || trilinear_auto);

	m_conf.tex = tex->m_texture;
	m_conf.pal = tex->m_palette;
}

GSRendererNew::PRIM_OVERLAP GSRendererNew::PrimitiveOverlap()
{
	// Either 1 triangle or 1 line or 3 POINTs
	// It is bad for the POINTs but low probability that they overlap
	if (m_vertex.next < 4)
		return PRIM_OVERLAP_NO;

	if (m_vt.m_primclass != GS_SPRITE_CLASS)
		return PRIM_OVERLAP_UNKNOW; // maybe, maybe not

	// Check intersection of sprite primitive only
	const size_t count = m_vertex.next;
	PRIM_OVERLAP overlap = PRIM_OVERLAP_NO;
	const GSVertex* v = m_vertex.buff;

	m_drawlist.clear();
	size_t i = 0;
	while (i < count)
	{
		// In order to speed up comparison a bounding-box is accumulated. It removes a
		// loop so code is much faster (check game virtua fighter). Besides it allow to check
		// properly the Y order.

		// .x = min(v[i].XYZ.X, v[i+1].XYZ.X)
		// .y = min(v[i].XYZ.Y, v[i+1].XYZ.Y)
		// .z = max(v[i].XYZ.X, v[i+1].XYZ.X)
		// .w = max(v[i].XYZ.Y, v[i+1].XYZ.Y)
		GSVector4i all = GSVector4i(v[i].m[1]).upl16(GSVector4i(v[i + 1].m[1])).upl16().xzyw();
		all = all.xyxy().blend(all.zwzw(), all > all.zwxy());

		size_t j = i + 2;
		while (j < count)
		{
			GSVector4i sprite = GSVector4i(v[j].m[1]).upl16(GSVector4i(v[j + 1].m[1])).upl16().xzyw();
			sprite = sprite.xyxy().blend(sprite.zwzw(), sprite > sprite.zwxy());

			// Be sure to get vertex in good order, otherwise .r* function doesn't
			// work as expected.
			ASSERT(sprite.x <= sprite.z);
			ASSERT(sprite.y <= sprite.w);
			ASSERT(all.x <= all.z);
			ASSERT(all.y <= all.w);

			if (all.rintersect(sprite).rempty())
			{
				all = all.runion_ordered(sprite);
			}
			else
			{
				overlap = PRIM_OVERLAP_YES;
				break;
			}
			j += 2;
		}
		m_drawlist.push_back((j - i) >> 1); // Sprite count
		i = j;
	}

#if 0
	// Old algo: less constraint but O(n^2) instead of O(n) as above

	// You have no guarantee on the sprite order, first vertex can be either top-left or bottom-left
	// There is a high probability that the draw call will uses same ordering for all vertices.
	// In order to keep a small performance impact only the first sprite will be checked
	//
	// Some safe-guard will be added in the outer-loop to avoid corruption with a limited perf impact
	if (v[1].XYZ.Y < v[0].XYZ.Y) {
		// First vertex is Top-Left
		for(size_t i = 0; i < count; i += 2) {
			if (v[i+1].XYZ.Y > v[i].XYZ.Y) {
				return PRIM_OVERLAP_UNKNOW;
			}
			GSVector4i vi(v[i].XYZ.X, v[i+1].XYZ.Y, v[i+1].XYZ.X, v[i].XYZ.Y);
			for (size_t j = i+2; j < count; j += 2) {
				GSVector4i vj(v[j].XYZ.X, v[j+1].XYZ.Y, v[j+1].XYZ.X, v[j].XYZ.Y);
				GSVector4i inter = vi.rintersect(vj);
				if (!inter.rempty()) {
					return PRIM_OVERLAP_YES;
				}
			}
		}
	} else {
		// First vertex is Bottom-Left
		for(size_t i = 0; i < count; i += 2) {
			if (v[i+1].XYZ.Y < v[i].XYZ.Y) {
				return PRIM_OVERLAP_UNKNOW;
			}
			GSVector4i vi(v[i].XYZ.X, v[i].XYZ.Y, v[i+1].XYZ.X, v[i+1].XYZ.Y);
			for (size_t j = i+2; j < count; j += 2) {
				GSVector4i vj(v[j].XYZ.X, v[j].XYZ.Y, v[j+1].XYZ.X, v[j+1].XYZ.Y);
				GSVector4i inter = vi.rintersect(vj);
				if (!inter.rempty()) {
					return PRIM_OVERLAP_YES;
				}
			}
		}
	}
#endif

	// fprintf(stderr, "%d: Yes, code can be optimized (draw of %d vertices)\n", s_n, count);
	return overlap;
}

void GSRendererNew::EmulateATST(float& AREF, GSHWDrawConfig::PSSelector& ps, bool pass_2)
{
	static const u32 inverted_atst[] = {ATST_ALWAYS, ATST_NEVER, ATST_GEQUAL, ATST_GREATER, ATST_NOTEQUAL, ATST_LESS, ATST_LEQUAL, ATST_EQUAL};

	if (!m_context->TEST.ATE)
		return;

	// Check for pass 2, otherwise do pass 1.
	const int atst = pass_2 ? inverted_atst[m_context->TEST.ATST] : m_context->TEST.ATST;


	switch (atst)
	{
		case ATST_LESS:
			AREF = static_cast<float>(m_context->TEST.AREF) - 0.1f;
			ps.atst = 1;
			break;
		case ATST_LEQUAL:
			AREF = static_cast<float>(m_context->TEST.AREF) - 0.1f + 1.0f;
			ps.atst = 1;
			break;
		case ATST_GEQUAL:
			AREF = static_cast<float>(m_context->TEST.AREF) - 0.1f;
			ps.atst = 2;
			break;
		case ATST_GREATER:
			AREF = static_cast<float>(m_context->TEST.AREF) - 0.1f + 1.0f;
			ps.atst = 2;
			break;
		case ATST_EQUAL:
			AREF = static_cast<float>(m_context->TEST.AREF);
			ps.atst = 3;
			break;
		case ATST_NOTEQUAL:
			AREF = static_cast<float>(m_context->TEST.AREF);
			ps.atst = 4;
			break;
		case ATST_NEVER: // Draw won't be done so no need to implement it in shader
		case ATST_ALWAYS:
		default:
			ps.atst = 0;
			break;
	}
}

void GSRendererNew::ResetStates()
{
	// We don't want to zero out the constant buffers, since fields used by the current draw could result in redundant uploads.
	// This memset should be pretty efficient - the struct is 16 byte aligned, as is the cb_vs offset.
	memset(&m_conf, 0, reinterpret_cast<const char*>(&m_conf.cb_vs) - reinterpret_cast<const char*>(&m_conf));
}

void GSRendererNew::DrawPrims(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex)
{
#ifdef ENABLE_OGL_DEBUG
	GSVector4i area_out = GSVector4i(m_vt.m_min.p.xyxy(m_vt.m_max.p)).rintersect(GSVector4i(m_context->scissor.in));
	GSVector4i area_in  = GSVector4i(m_vt.m_min.t.xyxy(m_vt.m_max.t));

	GL_PUSH("GL Draw from %d (area %d,%d => %d,%d) in %d (Depth %d) (area %d,%d => %d,%d)",
		tex && tex->m_texture ? tex->m_texture->GetID() : -1,
		area_in.x, area_in.y, area_in.z, area_in.w,
		rt ? rt->GetID() : -1, ds ? ds->GetID() : -1,
		area_out.x, area_out.y, area_out.z, area_out.w);
#endif

	const GSVector2i& rtsize = ds ? ds->GetSize()  : rt->GetSize();
	const GSVector2& rtscale = ds ? ds->GetScale() : rt->GetScale();

	const bool DATE = m_context->TEST.DATE && m_context->FRAME.PSM != PSM_PSMCT24;
	bool DATE_PRIMID = false;
	bool DATE_BARRIER = false;
	bool DATE_one  = false;

	const bool ate_first_pass = m_context->TEST.DoFirstPass();
	const bool ate_second_pass = m_context->TEST.DoSecondPass();

	ResetStates();
	m_conf.cb_vs.texture_offset = GSVector2(0, 0);
	m_conf.ps.scanmsk = m_env.SCANMSK.MSK;

	ASSERT(g_gs_device != NULL);

	// HLE implementation of the channel selection effect
	//
	// Warning it must be done at the begining because it will change the
	// vertex list (it will interact with PrimitiveOverlap and accurate
	// blending)
	EmulateChannelShuffle(&rt, tex);

	// Upscaling hack to avoid various line/grid issues
	MergeSprite(tex);

	// Always check if primitive overlap as it is used in plenty of effects.
	if (g_gs_device->Features().texture_barrier)
		m_prim_overlap = PrimitiveOverlap();
	else
		m_prim_overlap = PRIM_OVERLAP_UNKNOW; // Prim overlap check is useless without texture barrier

	// Detect framebuffer read that will need special handling
	if (g_gs_device->Features().texture_barrier && (m_context->FRAME.Block() == m_context->TEX0.TBP0) && PRIM->TME && GSConfig.AccurateBlendingUnit != AccBlendLevel::Minimum)
	{
		if ((m_context->FRAME.FBMSK == 0x00FFFFFF) && (m_vt.m_primclass == GS_TRIANGLE_CLASS))
		{
			// This pattern is used by several games to emulate a stencil (shadow)
			// Ratchet & Clank, Jak do alpha integer multiplication (tfx) which is mostly equivalent to +1/-1
			// Tri-Ace (Star Ocean 3/RadiataStories/VP2) uses a palette to handle the +1/-1
			GL_DBG("Source and Target are the same! Let's sample the framebuffer");
			m_conf.ps.tex_is_fb = 1;
			m_conf.require_full_barrier = true;
		}
		else if (m_prim_overlap != PRIM_OVERLAP_NO)
		{
			// Note: It is fine if the texture fits in a single GS page. First access will cache
			// the page in the GS texture buffer.
			GL_INS("ERROR: Source and Target are the same!");
		}
	}

	EmulateTextureShuffleAndFbmask();

	// DATE: selection of the algorithm. Must be done before blending because GL42 is not compatible with blending
	if (DATE)
	{
		// It is way too complex to emulate texture shuffle with DATE. So just use
		// the slow but accurate algo
		const bool fbmask = (m_context->FRAME.FBMSK & 0x80000000);
		const bool no_overlap = (m_prim_overlap == PRIM_OVERLAP_NO);
		if (fbmask || no_overlap || m_texture_shuffle)
		{
			GL_PERF("DATE: Accurate with %s", m_texture_shuffle ? "texture shuffle" : no_overlap ? "no overlap" : "FBMASK");
			if (g_gs_device->Features().texture_barrier)
			{
				m_conf.require_full_barrier = true;
				DATE_BARRIER = true;
			}
		}
		else if (m_context->FBA.FBA)
		{
			DATE_one = !m_context->TEST.DATM;
			GL_PERF("DATE: Fast with FBA, all pixels will be >= 128");
		}
		else if (m_conf.colormask.wa && !m_context->TEST.ATE)
		{
			// Performance note: check alpha range with GetAlphaMinMax()
			// Note: all my dump are already above 120fps, but it seems to reduce GPU load
			// with big upscaling
			if (m_context->TEST.DATM && GetAlphaMinMax().max < 128)
			{
				// Only first pixel (write 0) will pass (alpha is 1)
				GL_PERF("DATE: Fast with alpha %d-%d", GetAlphaMinMax().min, GetAlphaMinMax().max);
				DATE_one = true;
			}
			else if (!m_context->TEST.DATM && GetAlphaMinMax().min >= 128)
			{
				// Only first pixel (write 1) will pass (alpha is 0)
				GL_PERF("DATE: Fast with alpha %d-%d", GetAlphaMinMax().min, GetAlphaMinMax().max);
				DATE_one = true;
			}
			else if ((m_vt.m_primclass == GS_SPRITE_CLASS && m_drawlist.size() < 50) || (m_index.tail < 100))
			{
				// texture barrier will split the draw call into n draw call. It is very efficient for
				// few primitive draws. Otherwise it sucks.
				GL_PERF("DATE: Accurate with alpha %d-%d", GetAlphaMinMax().min, GetAlphaMinMax().max);
				if (g_gs_device->Features().texture_barrier)
				{
					m_conf.require_full_barrier = true;
					DATE_BARRIER = true;
				}
			}
			else if (GSConfig.AccurateDATE)
			{
				// Note: Fast level (DATE_one) was removed as it's less accurate.
				GL_PERF("DATE: Accurate with alpha %d-%d", GetAlphaMinMax().min, GetAlphaMinMax().max);
				if (g_gs_device->Features().image_load_store)
				{
					DATE_PRIMID = true;
				}
				else if (g_gs_device->Features().texture_barrier)
				{
					m_conf.require_full_barrier = true;
					DATE_BARRIER = true;
				}
				else
				{
					DATE_one = true;
				}
			}
		}
		else if (!m_conf.colormask.wa && !m_context->TEST.ATE)
		{
			// TODO: is it legal ? Likely but it need to be tested carefully
			// DATE_BARRIER = true;
			// m_conf.require_one_barrier = true; << replace it with a cheap barrier
		}

		// Will save my life !
		ASSERT(!(DATE_BARRIER && DATE_one));
		ASSERT(!(DATE_PRIMID && DATE_one));
		ASSERT(!(DATE_PRIMID && DATE_BARRIER));
	}

	// Blend

	if (!IsOpaque() && rt)
	{
		EmulateBlending(DATE_PRIMID, DATE_BARRIER);
	}
	else
	{
		m_conf.blend = {}; // No blending please
	}

	if (m_conf.ps.scanmsk & 2)
		DATE_PRIMID = false; // to have discard in the shader work correctly

	if (m_conf.ps.dfmt == 1)
	{
		// Disable writing of the alpha channel
		m_conf.colormask.wa = 0;
	}

	// DATE setup, no DATE_BARRIER please

	if (!DATE)
		m_conf.destination_alpha = GSHWDrawConfig::DestinationAlphaMode::Off;
	else if (DATE_one)
		m_conf.destination_alpha = GSHWDrawConfig::DestinationAlphaMode::StencilOne;
	else if (DATE_PRIMID)
		m_conf.destination_alpha = GSHWDrawConfig::DestinationAlphaMode::PrimIDTracking;
	else if (DATE_BARRIER)
		m_conf.destination_alpha = GSHWDrawConfig::DestinationAlphaMode::Full;
	else
		m_conf.destination_alpha = GSHWDrawConfig::DestinationAlphaMode::Stencil;

	m_conf.datm = m_context->TEST.DATM;

	// om

	EmulateZbuffer(); // will update VS depth mask

	// vs

	m_conf.vs.tme = PRIM->TME;
	m_conf.vs.fst = PRIM->FST;

	// FIXME D3D11 and GL support half pixel center. Code could be easier!!!
	const float sx = 2.0f * rtscale.x / (rtsize.x << 4);
	const float sy = 2.0f * rtscale.y / (rtsize.y << 4);
	const float ox = (float)(int)m_context->XYOFFSET.OFX;
	const float oy = (float)(int)m_context->XYOFFSET.OFY;
	float ox2 = -1.0f / rtsize.x;
	float oy2 = -1.0f / rtsize.y;

	//This hack subtracts around half a pixel from OFX and OFY.
	//
	//The resulting shifted output aligns better with common blending / corona / blurring effects,
	//but introduces a few bad pixels on the edges.

	if (rt && rt->LikelyOffset && GSConfig.UserHacks_HalfPixelOffset == 1)
	{
		ox2 *= rt->OffsetHack_modx;
		oy2 *= rt->OffsetHack_mody;
	}

	m_conf.cb_vs.vertex_scale = GSVector2(sx, sy);
	m_conf.cb_vs.vertex_offset = GSVector2(ox * sx + ox2 + 1, oy * sy + oy2 + 1);
	// END of FIXME

	// GS_SPRITE_CLASS are already flat (either by CPU or the GS)
	m_conf.ps.iip = (m_vt.m_primclass == GS_SPRITE_CLASS) ? 0 : PRIM->IIP;
	m_conf.gs.iip = m_conf.ps.iip;
	m_conf.vs.iip = m_conf.ps.iip;

	if (DATE_BARRIER)
	{
		m_conf.ps.date = 5 + m_context->TEST.DATM;
	}
	else if (DATE_one)
	{
		if (g_gs_device->Features().texture_barrier)
		{
			m_conf.require_one_barrier = true;
			m_conf.ps.date = 5 + m_context->TEST.DATM;
		}
		m_conf.depth.date = 1;
		m_conf.depth.date_one = 1;
	}
	else if (DATE)
	{
		if (DATE_PRIMID)
			m_conf.ps.date = 1 + m_context->TEST.DATM;
		else
			m_conf.depth.date = 1;
	}

	m_conf.ps.fba = m_context->FBA.FBA;
	m_conf.ps.dither = GSConfig.Dithering > 0 && m_conf.ps.dfmt == 2 && m_env.DTHE.DTHE;

	if (m_conf.ps.dither)
	{
		GL_DBG("DITHERING mode ENABLED (%d)", m_dithering);

		m_conf.ps.dither = GSConfig.Dithering;
		m_conf.cb_ps.DitherMatrix[0] = GSVector4(m_env.DIMX.DM00, m_env.DIMX.DM01, m_env.DIMX.DM02, m_env.DIMX.DM03);
		m_conf.cb_ps.DitherMatrix[1] = GSVector4(m_env.DIMX.DM10, m_env.DIMX.DM11, m_env.DIMX.DM12, m_env.DIMX.DM13);
		m_conf.cb_ps.DitherMatrix[2] = GSVector4(m_env.DIMX.DM20, m_env.DIMX.DM21, m_env.DIMX.DM22, m_env.DIMX.DM23);
		m_conf.cb_ps.DitherMatrix[3] = GSVector4(m_env.DIMX.DM30, m_env.DIMX.DM31, m_env.DIMX.DM32, m_env.DIMX.DM33);
	}

	if (PRIM->FGE)
	{
		m_conf.ps.fog = 1;

		const GSVector4 fc = GSVector4::rgba32(m_env.FOGCOL.U32[0]);
		// Blend AREF to avoid to load a random value for alpha (dirty cache)
		m_conf.cb_ps.FogColor_AREF = fc.blend32<8>(m_conf.cb_ps.FogColor_AREF);
	}

	// Warning must be done after EmulateZbuffer
	// Depth test is always true so it can be executed in 2 passes (no order required) unlike color.
	// The idea is to compute first the color which is independent of the alpha test. And then do a 2nd
	// pass to handle the depth based on the alpha test.
	bool ate_RGBA_then_Z = false;
	bool ate_RGB_then_ZA = false;
	if (ate_first_pass & ate_second_pass)
	{
		GL_DBG("Complex Alpha Test");
		const bool commutative_depth = (m_conf.depth.ztst == ZTST_GEQUAL && m_vt.m_eq.z) || (m_conf.depth.ztst == ZTST_ALWAYS);
		const bool commutative_alpha = (m_context->ALPHA.C != 1); // when either Alpha Src or a constant

		ate_RGBA_then_Z = (m_context->TEST.AFAIL == AFAIL_FB_ONLY) & commutative_depth;
		ate_RGB_then_ZA = (m_context->TEST.AFAIL == AFAIL_RGB_ONLY) & commutative_depth & commutative_alpha;
	}

	if (ate_RGBA_then_Z)
	{
		GL_DBG("Alternate ATE handling: ate_RGBA_then_Z");
		// Render all color but don't update depth
		// ATE is disabled here
		m_conf.depth.zwe = false;
	}
	else if (ate_RGB_then_ZA)
	{
		GL_DBG("Alternate ATE handling: ate_RGB_then_ZA");
		// Render RGB color but don't update depth/alpha
		// ATE is disabled here
		m_conf.depth.zwe = false;
		m_conf.colormask.wa = false;
	}
	else
	{
		float aref = m_conf.cb_ps.FogColor_AREF.a;
		EmulateATST(aref, m_conf.ps, false);

		// avoid redundant cbuffer updates
		m_conf.cb_ps.FogColor_AREF.a = aref;
		m_conf.alpha_second_pass.ps_aref = aref;
	}

	if (tex)
	{
		EmulateTextureSampler(tex);
	}
	else
	{
		m_conf.ps.tfx = 4;
	}

	if (m_game.title == CRC::ICO)
	{
		const GSVertex* v = &m_vertex.buff[0];
		const GSVideoMode mode = GetVideoMode();
		if (tex && m_vt.m_primclass == GS_SPRITE_CLASS && m_vertex.next == 2 && PRIM->ABE && // Blend texture
			((v[1].U == 8200 && v[1].V == 7176 && mode == GSVideoMode::NTSC) || // at display resolution 512x448
			(v[1].U == 8200 && v[1].V == 8200 && mode == GSVideoMode::PAL)) && // at display resolution 512x512
			tex->m_TEX0.PSM == PSM_PSMT8H) // i.e. read the alpha channel of a 32 bits texture
		{
			// Note potentially we can limit to TBP0:0x2800

			// Depth buffer was moved so GS will invalide it which means a
			// downscale. ICO uses the MSB depth bits as the texture alpha
			// channel.  However this depth of field effect requires
			// texel:pixel mapping accuracy.
			//
			// Use an HLE shader to sample depth directly as the alpha channel
			GL_INS("ICO sample depth as alpha");
			m_conf.require_full_barrier = true;
			// Extract the depth as palette index
			m_conf.ps.depth_fmt = 1;
			m_conf.ps.channel = ChannelFetch_BLUE;
			m_conf.raw_tex = ds;

			// We need the palette to convert the depth to the correct alpha value.
			if (!tex->m_palette)
			{
				const u16 pal = GSLocalMemory::m_psm[tex->m_TEX0.PSM].pal;
				m_tc->AttachPaletteToSource(tex, pal, true);
				m_conf.pal = tex->m_palette;
			}
		}
	}

	// rs
	const GSVector4& hacked_scissor = m_channel_shuffle ? GSVector4(0, 0, 1024, 1024) : m_context->scissor.in;
	const GSVector4i scissor = GSVector4i(GSVector4(rtscale).xyxy() * hacked_scissor).rintersect(GSVector4i(rtsize).zwxy());

	m_conf.drawarea = scissor.rintersect(ComputeBoundingBox(rtscale, rtsize));
	m_conf.scissor = (DATE && !DATE_BARRIER) ? m_conf.drawarea : scissor;

	SetupIA(sx, sy);

	if (rt)
		rt->CommitRegion(GSVector2i(m_conf.drawarea.z, m_conf.drawarea.w));

	if (ds)
		ds->CommitRegion(GSVector2i(m_conf.drawarea.z, m_conf.drawarea.w));

	m_conf.alpha_second_pass.enable = ate_second_pass;

	if (ate_second_pass)
	{
		ASSERT(!m_env.PABE.PABE);
		memcpy(&m_conf.alpha_second_pass.ps,        &m_conf.ps,        sizeof(m_conf.ps));
		memcpy(&m_conf.alpha_second_pass.colormask, &m_conf.colormask, sizeof(m_conf.colormask));
		memcpy(&m_conf.alpha_second_pass.depth,     &m_conf.depth,     sizeof(m_conf.depth));

		if (ate_RGBA_then_Z | ate_RGB_then_ZA)
		{
			// Enable ATE as first pass to update the depth
			// of pixels that passed the alpha test
			EmulateATST(m_conf.alpha_second_pass.ps_aref, m_conf.alpha_second_pass.ps, false);
		}
		else
		{
			// second pass will process the pixels that failed
			// the alpha test
			EmulateATST(m_conf.alpha_second_pass.ps_aref, m_conf.alpha_second_pass.ps, true);
		}


		bool z = m_conf.depth.zwe;
		bool r = m_conf.colormask.wr;
		bool g = m_conf.colormask.wg;
		bool b = m_conf.colormask.wb;
		bool a = m_conf.colormask.wa;

		switch (m_context->TEST.AFAIL)
		{
			case AFAIL_KEEP: z = r = g = b = a = false; break; // none
			case AFAIL_FB_ONLY: z = false; break; // rgba
			case AFAIL_ZB_ONLY: r = g = b = a = false; break; // z
			case AFAIL_RGB_ONLY: z = a = false; break; // rgb
			default: __assume(0);
		}

		// Depth test should be disabled when depth writes are masked and similarly, Alpha test must be disabled
		// when writes to all of the alpha bits in the Framebuffer are masked.
		if (ate_RGBA_then_Z)
		{
			z = !m_context->ZBUF.ZMSK;
			r = g = b = a = false;
		}
		else if (ate_RGB_then_ZA)
		{
			z = !m_context->ZBUF.ZMSK;
			a = (m_context->FRAME.FBMSK & 0xFF000000) != 0xFF000000;
			r = g = b = false;
		}

		if (z || r || g || b || a)
		{
			m_conf.alpha_second_pass.depth.zwe = z;
			m_conf.alpha_second_pass.colormask.wr = r;
			m_conf.alpha_second_pass.colormask.wg = g;
			m_conf.alpha_second_pass.colormask.wb = b;
			m_conf.alpha_second_pass.colormask.wa = a;
		}
		else
		{
			m_conf.alpha_second_pass.enable = false;
		}
	}

	if (!ate_first_pass)
	{
		if (!m_conf.alpha_second_pass.enable)
			return;

		// RenderHW always renders first pass, replace first pass with second
		memcpy(&m_conf.ps,        &m_conf.alpha_second_pass.ps,        sizeof(m_conf.ps));
		memcpy(&m_conf.colormask, &m_conf.alpha_second_pass.colormask, sizeof(m_conf.colormask));
		memcpy(&m_conf.depth,     &m_conf.alpha_second_pass.depth,     sizeof(m_conf.depth));
		m_conf.cb_ps.FogColor_AREF.a = m_conf.alpha_second_pass.ps_aref;
		m_conf.alpha_second_pass.enable = false;
	}

	if (m_conf.require_full_barrier && m_prim_overlap == PRIM_OVERLAP_NO)
	{
		m_conf.require_full_barrier = false;
		m_conf.require_one_barrier = true;
	}

	m_conf.drawlist = (m_conf.require_full_barrier && m_vt.m_primclass == GS_SPRITE_CLASS) ? &m_drawlist : nullptr;

	m_conf.rt = rt;
	m_conf.ds = ds;
	g_gs_device->RenderHW(m_conf);
}

bool GSRendererNew::IsDummyTexture() const
{
	// Texture is actually the frame buffer. Stencil emulation to compute shadow (Jak series/tri-ace game)
	// Will hit the "m_ps_sel.tex_is_fb = 1" path in the draw
	return g_gs_device->Features().texture_barrier && (m_context->FRAME.Block() == m_context->TEX0.TBP0) && PRIM->TME && GSConfig.AccurateBlendingUnit != AccBlendLevel::Minimum && m_vt.m_primclass == GS_TRIANGLE_CLASS && (m_context->FRAME.FBMSK == 0x00FFFFFF);
}
