/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSdx.h"
#include "GSCrc.h"

CRC::Game CRC::m_games[] =
{
	// Note: Id 0x7ACF7E03 shouldn't be added as it's from the multiloader when packing games.
	{0x00000000, NoTitle, NoRegion, 0},
	{0xF46142D3, ArTonelico2, NoRegion, 0},
	{0xC38067F4, ArTonelico2, NoRegion, 0}, // project metafalica 1.0
	{0xF95F37EE, ArTonelico2, US, 0},
	{0xCE2C1DBF, ArTonelico2, EU, 0},
	{0x2113EA2E, MetalSlug6, JP, 0},
	{0x42E05BAF, TomoyoAfter, JP, PointListPalette},
	{0x7800DC84, Clannad, JP, PointListPalette},
	{0xA6167B59, Lamune, JP, PointListPalette},
	{0xDDB59F46, KyuuketsuKitanMoonties, JP, PointListPalette},
	{0xC8EE2562, PiaCarroteYoukosoGPGakuenPrincess, JP, PointListPalette},
	{0x6CF94A43, KazokuKeikakuKokoroNoKizuna, JP, PointListPalette},
	{0xEDAF602D, DuelSaviorDestiny, JP, PointListPalette},
	{0xA39517AB, FFX, EU, 0},
	{0xA39517AE, FFX, FR, 0},
	{0x941BB7D9, FFX, DE, 0},
	{0xA39517A9, FFX, IT, 0},
	{0x941BB7DE, FFX, ES, 0},
	{0xA80F497C, FFX, ES, 0},
	{0xB4414EA1, FFX, RU, 0},
	{0xEE97DB5B, FFX, RU, 0},
	{0xAEC495CC, FFX, RU, 0},
	{0xBB3D833A, FFX, US, 0},
	{0x6A4EFE60, FFX, JP, 0},
	{0x3866CA7E, FFX, ASIA, 0}, // int.
	{0x658597E2, FFX, JP, 0}, // int.
	{0x9AAC5309, FFX2, EU, 0},
	{0x9AAC530C, FFX2, FR, 0},
	{0x9AAC530A, FFX2, ES, 0},
	{0x9AAC530D, FFX2, DE, 0},
	{0x9AAC530B, FFX2, IT, 0},
	{0x48FE0C71, FFX2, US, 0},
	{0x8A6D7F14, FFX2, JP, 0},
	{0xE1FD9A2D, FFX2, JP, 0}, // int.
	{0x11624CD6, FFX2, KO, 0},
	{0x78DA0252, FFXII, EU, 0},
	{0xC1274668, FFXII, EU, 0},
	{0xDC2A467E, FFXII, EU, 0},
	{0xCA284668, FFXII, EU, 0},
	{0xC52B466E, FFXII, EU, 0}, // ES
	{0xE5E71BF9, FFXII, FR, 0},
	{0x0779FBDB, FFXII, US, 0},
	{0x280AD120, FFXII, JP, 0},
	{0x08C1ED4D, HauntingGround, EU, 0},
	{0x2CD5794C, HauntingGround, EU, 0},
	// {0x7D4EA48F, HauntingGround, EU, 0}, // same CRC as {Genji, EU}
	{0x867BB945, HauntingGround, JP, 0},
	{0xE263BC4B, HauntingGround, JP, 0},
	{0x901AAC09, HauntingGround, US, 0},
	{0x21068223, Okami, US, 0},
	{0x891F223F, Okami, EU, 0}, // PAL DE, ES & FR.
	{0xC5DEFEA0, Okami, JP, 0},
	{0x086273D2, MetalGearSolid3, EU, 0}, // - PAL UK & FR
	{0x26A6E286, MetalGearSolid3, DE, 0},
	{0x9F185CE1, MetalGearSolid3, EU, 0},
	{0x98D4BC93, MetalGearSolid3, ES, 0},
	{0x79ED26AD, MetalGearSolid3, EU, 0},
	{0x5E31EA42, MetalGearSolid3, EU, 0},
	{0xD7ED797D, MetalGearSolid3, DE, 0},
	{0x053D2239, MetalGearSolid3, US, 0}, // Metal Gear Solid 3 Subsistence disc1
	{0x01B2FA7F, MetalGearSolid3, US, 0}, // Metal Gear Solid 3 Subsistence disc2
	{0xAA31B5BF, MetalGearSolid3, US, 0},
	{0x86BC3040, MetalGearSolid3, US, 0}, // Metal Gear Solid 3 Subsistence disc1
	{0x0481AD8A, MetalGearSolid3, JP, 0},
	{0xC69ACB6F, MetalGearSolid3, KO, 0}, // Metal Gear Solid 3 Snake Eater
	{0xB0D195EF, MetalGearSolid3, KO, 0}, // Metal Gear Solid 3 Subsistence disc1
	{0x3EBABC9C, MetalGearSolid3, KO, 0}, // Metal Gear Solid 3 Subsistence disc2
	{0x8A5C25A7, MetalGearSolid3, ES, 0}, // Metal Gear Solid 3 Subsistence Spanish version
	{0x278722BF, DBZBT2, EU, 0},
	{0xFE961D28, DBZBT2, US, 0},
	{0x0393B6BE, DBZBT2, EU, 0},
	{0xE2F289ED, DBZBT2, JP, 0}, // Sparking Neo!
	{0xE29C09A3, DBZBT2, KO, 0}, // DragonBall Z Sparking Neo
	{0x0BAA4387, DBZBT2, JP, 0},
	{0x35AA84D1, DBZBT2, NoRegion, 0},
	{0xBE6A9CFB, DBZBT2, NoRegion, 0},
	{0x428113C2, DBZBT3, US, 0},
	{0xA422BB13, DBZBT3, EU, 0},
	{0xCE93CB30, DBZBT3, JP, 0},
	{0xF28D21F1, DBZBT3, JP, 0},
	{0x983C53D2, DBZBT3, NoRegion, 0},
	{0x983C53D3, DBZBT3, EU, 0},
	{0x9B0E119F, DBZBT3, KO, 0}, // DragonBall Z Sparking Meteo
	{0x5E13E6D6, SFEX3, EU, 0},
	{0x72B3802A, SFEX3, US, 0},
	{0x71521863, SFEX3, US, 0},
	{0x63642E9F, SFEX3, JP, 0},
	{0xCA1F6E53, SFEX3, JP, 0}, // Taikenban Disc (Demo/Trial)
	{0x28703748, Bully, US, 0},
	{0x019CFA48, Bully, JP, 0},
	{0xC78A495D, BullyCC, EU, 0},
	{0xC19A374E, SoTC, US, 0},
	{0x7D8F539A, SoTC, EU, 0},
	{0x0F0C4A9C, SoTC, EU, 0},
	{0x877F3436, SoTC, JP, 0},
	{0xA17D6AAA, SoTC, KO, 0},
	{0x877B3D35, SoTC, CH, 0},
	{0x3122B508, OnePieceGrandAdventure, US, 0},
	{0x8DF14A24, OnePieceGrandAdventure, EU, 0},
	{0xE446C9F9, OnePieceGrandAdventure, KO, 0},
	{0xCA2073B3, OnePieceGrandBattle, KO, 0},
	{0x66953267, OnePieceGrandAdventure, JP, 0},
	{0xE1674F57, OnePieceGrandBattle, EU, 0},
	{0x947B933B, OnePieceGrandAdventure, US, 0},
	{0xB049DD5E, OnePieceGrandBattle, US, 0},
	{0x5D02CC5B, OnePieceGrandBattle, NoRegion, 0},
	{0x6F8545DB, ICO, US, 0},
	{0x48CDF317, ICO, US, 0}, // Demo
	{0xB01A4C95, ICO, JP, 0},
	{0x2DF2C1EA, ICO, KO, 0},
	{0x5C991F4E, ICO, EU, 0},
	{0x3DCE2229, ICO, RU, 0}, // Unofficial RU-version
	{0x788D8B4F, ICO, EU, 0},
	{0x29C28734, ICO, CH, 0},
	{0xC02C653E, GT4, CH, 0},
	{0x7ABDBB5E, GT4, CH, 0}, // cutie comment
	{0xAEAD1CA3, GT4, JP, 0},
	{0xE906EA37, GT4, JP, 0}, // GT4 First Preview 
	{0xCA6243B9, GT4, JP, 0}, // GT4 Prologue
	{0xDD764BBE, GT4, JP, 0}, // GT4 Prologue
	{0x30E41D93, GT4, KO, 0},
	{0x715CF2EC, GT4, EU, 0},
	{0x44A61C8F, GT4, EU, 0},
	{0x0086E35B, GT4, EU, 0},
	{0x3FB69323, GT4, EU, 0}, // GT4 Prologue
	{0x77E61C8A, GT4, US, 0},
	{0x33C6E35E, GT4, US, 0},
	{0x32A1C752, GT4, US, 0}, // GT4 Online Beta
	{0x70538747, GT4, US, 0}, // Toyota Prius Trial
	{0x3E9D448A, GT3, CH, 0}, // cutie comment
	{0xAD66643C, GT3, CH, 0}, // cutie comment
	{0x85AE91B3, GT3, US, 0},
	{0x8AA991B0, GT3, US, 0},
	{0xC220951A, GT3, JP, 0},
	{0x9DE5CF65, GT3, JP, 0},
	{0xB590CE04, GT3, EU, 0},
	{0x60013EBD, GTConcept, EU, 0},
	{0x6810C3BC, GTConcept, CH, 0}, // Gran Turismo Concept 2002 Tokyo-Geneva
	{0x0EEF32A3, GTConcept, KO, 0}, // Gran Turismo Concept 2002 Tokyo-Seoul
	{0xC164550A, WildArms5, JPUNDUB, 0},
	{0xC1640D2C, WildArms5, US, 0},
	{0x0FCF8FE4, WildArms5, EU, 0},
	{0x2294D322, WildArms5, JP, 0},
	{0x565B6170, WildArms4, JP, 0}, // Wild Arms: The 4th Detonator
	{0xBBC3EFFA, WildArms4, US, 0},
	{0xBBC396EC, WildArms4, US, 0}, // hmm such a small diff in the CRC..
	{0x7B2DE9CC, WildArms4, EU, 0},
	{0x8B029334, Manhunt2, EU, 0},
	{0x3B0ADBEF, Manhunt2, US, 0},
	{0x09F49E37, CrashBandicootWoC, NoRegion, 0},
	{0x103B5706, CrashBandicootWoC, US, 0}, // American Greatest Hits release
	{0x75182BE5, CrashBandicootWoC, US, 0},
	{0x5188ABCA, CrashBandicootWoC, US, 0},
	{0x34E2EEC7, CrashBandicootWoC, RU, 0},
	{0x3A03D62F, CrashBandicootWoC, EU, 0},
	{0x013E349D, ResidentEvil4, US, 0},
	{0xDBB7A559, ResidentEvil4, US, 0},
	{0x6BA2F6B9, ResidentEvil4, EU, 0},
	{0x60FA8C69, ResidentEvil4, JP, 0},
	{0x5F254B7C, ResidentEvil4, KO, 0},
	{0x72E1E60E, Spartan, EU, 0},
	{0x26689C87, Spartan, JP, 0},
	{0x08277A9E, Spartan, US, 0},
	{0xAC3C1147, SVCChaos, EU, 0}, // SVC Chaos: SNK vs. Capcom
	{0xB00FF2ED, SVCChaos, JP, 0},
	{0xA32F7CD0, AceCombat4, US, 0}, // Also needed for automatic mipmapping
	{0x5ED8FB53, AceCombat4, JP, 0},
	{0x1B9B7563, AceCombat4, EU, 0},
	{0x39B574F0, AceCombat5, US, 0}, // The Unsung War
	{0x86089F31, AceCombat5, JP, 0},
	{0x1D54FEA9, AceCombat5, EU, 0}, // Squadron Leader
	{0xFC46EA61, Tekken5, JP, 0},
	{0x1F88EE37, Tekken5, EU, 0},
	{0x1F88BECD, Tekken5, EU, 0}, // language selector...
	{0x652050D2, Tekken5, US, 0},
	{0xEA64EF39, Tekken5, KO, 0},
	{0x9E98B8AE, IkkiTousen, JP, 0},
	{0xD6385328, GodOfWar, US, 0},
	{0xF2A8D307, GodOfWar, US, 0},
	{0xA61A4C6D, GodOfWar, US, 0},
	{0xDF3A5A5C, GodOfWar, US, 0}, // Demo
	{0xFB0E6D72, GodOfWar, EU, 0},
	{0xEB001875, GodOfWar, EU, 0},
	{0xCF148C74, GodOfWar, EU, 0},
	{0xDF1AF973, GodOfWar, EU, 0},
	{0xCA052D22, GodOfWar, JP, 0},
	{0xBFCC1795, GodOfWar, KO, 0},
	{0x9567B7D6, GodOfWar, KO, 0},
	{0x9B5C97BA, GodOfWar, KO, 0},
	{0xE23D532B, GodOfWar, NoRegion, 0},
	{0x1A85E924, GodOfWar, NoRegion, 0}, // cutie comment
	{0x608ACBD3, GodOfWar, CH, 0}, // cutie comment
	{0x2F123FD8, GodOfWar2, US, 0}, // same CRC as RU
	{0x44A8A22A, GodOfWar2, EU, 0},
	{0x60BC362B, GodOfWar2, EU, 0},
	{0x4340C7C6, GodOfWar2, KO, 0},
	{0xE96E55BD, GodOfWar2, JP, 0},
	{0xF8CD3DF6, GodOfWar2, NoRegion, 0},
	{0x0B82BFF7, GodOfWar2, NoRegion, 0},
	{0x5990866F, GodOfWar2, NoRegion, 0},
	{0xC4C4FD5F, GodOfWar2, CH, 0},
	{0xDCD9A9F7, GodOfWar2, EU, 0},
	{0xFA0DF523, GodOfWar2, CH, 0}, // cutie comment
	{0x9FEE3466, GodOfWar2, CH, 0}, // cutie comment
	{0x5D482F18, JackieChanAdv, EU, 0},
	{0xAC4DFD5A, JackieChanAdv, EU, 0},
	{0x95CC86EF, GiTS, US, 0}, // same CRC also reported as EU
	{0x2C5BF134, GiTS, US, 0}, // Demo
	{0xA5768F53, GiTS, JP, 0},
	{0xA3643EB1, GiTS, KO, 0},
	{0x28557423, GiTS, RU, 0},
	{0xBF6F101F, GiTS, EU, 0}, // same CRC as another US disc
	{0x6BF11378, Onimusha3, US, 0},
	{0x78F1136A, Onimusha3, RU, 0}, // Unofficial RU-version
	{0x71320CA8, Onimusha3, JP, 0},
	{0xDAFFFB0D, Onimusha3, KO, 0},
	{0xF442260C, MajokkoALaMode2, JP, 0},
	{0x14FE77F7, TalesOfAbyss, US, 0},
	{0x045D77E9, TalesOfAbyss, JPUNDUB, 0},
	{0xAA5EC3A3, TalesOfAbyss, JP, 0},
	{0xFB236A46, SonicUnleashed, US, 0},
	{0x8C913264, SonicUnleashed, EU, 0},
	{0x5C1EBD61, SimpsonsGame, EU, 0},
	{0x5C1EBF61, SimpsonsGame, FR, 0},
	{0x4C7BB3C8, SimpsonsGame, NoRegion, 0},
	{0x4C94B32C, SimpsonsGame, NoRegion, 0},
	{0x565B7E04, SimpsonsGame, IT, 0},
	{0x206779D8, SimpsonsGame, EU, 0},
	{0xBBE4D862, SimpsonsGame, US, 0},
	{0xD71B57F4, Genji, US, 0},
	{0xFADEBC45, Genji, EU, 0},
	{0xB4776FC1, Genji, JP, 0},
	{0x56242EC9, Genji, KO, 0},
	{0xCDAF243D, Genji, CH, 0},
	{0x2A5E0B61, Genji, CH, 0},
	{0x7D4EA48F, Genji, EU, 0}, // same CRC as {HauntingGround, EU}
	{0xE04EA200, StarOcean3, EU, 0},
	{0x23A97857, StarOcean3, US, 0},
	{0xBEC32D49, StarOcean3, JP, 0},
	{0x8192A241, StarOcean3, JP, 0}, // NTSC JP special directors cut limited extra sugar on top edition (the special one :p)
	// it's the US version with speach files from JP... {0x23A97857, StarOcean3, JPUNDUB, 0},
	{0xCC96CE93, ValkyrieProfile2, US, 0},
	{0x774DE8E2, ValkyrieProfile2, JP, 0},
	{0x04CCB600, ValkyrieProfile2, EU, 0},
	{0xB65E141B, ValkyrieProfile2, DE, 0}, // PAL German
	{0x8510854E, ValkyrieProfile2, FR, 0},
	{0xC70FC973, ValkyrieProfile2, IT, 0},
	{0x47B9B2FD, RadiataStories, US, 0},
	{0xAC73005E, RadiataStories, JP, 0},
	{0xE8FCF8EC, SMTNocturne, US, ZWriteMustNotClear}, // saves/reloads z buffer around shadow drawing, same issue with all the SMT games following
	{0xF0A31EE3, SMTNocturne, EU, ZWriteMustNotClear}, // SMTNocturne (Lucifers Call in EU)
	{0xAE0DE7B7, SMTNocturne, EU, ZWriteMustNotClear}, // SMTNocturne (Lucifers Call in EU)
	{0xD60DA6D4, SMTNocturne, JP, ZWriteMustNotClear}, // SMTNocturne
	{0x0E762E8D, SMTNocturne, JP, ZWriteMustNotClear}, // SMTNocturne Maniacs
	{0x47BA9034, SMTNocturne, JP, ZWriteMustNotClear}, // SMTNocturne Maniacs Chronicle
	{0xD3FFC263, SMTNocturne, KO, ZWriteMustNotClear},
	{0xD7273511, SMTDDS1, US, ZWriteMustNotClear}, // SMT Digital Devil Saga
	{0x1683A6BE, SMTDDS1, EU, ZWriteMustNotClear}, // SMT Digital Devil Saga
	{0x44865CE1, SMTDDS1, JP, ZWriteMustNotClear}, // SMT Digital Devil Saga
	{0xF2E397C0, SMTDDS1, KO, ZWriteMustNotClear}, // SMT Digital Devil Saga
	{0x43202D1A, SMTDDS2, KO, ZWriteMustNotClear}, // SMT Digital Devil Saga 2
	{0xD382C164, SMTDDS2, US, ZWriteMustNotClear}, // SMT Digital Devil Saga 2
	{0xD568B684, SMTDDS2, EU, ZWriteMustNotClear}, // SMT Digital Devil Saga 2
	{0xE47C1A9C, SMTDDS2, JP, ZWriteMustNotClear}, // SMT Digital Devil Saga 2
	{0x0B8AB37B, RozenMaidenGebetGarden, JP, 0},
	{0x1CC39DBD, SuikodenTactics, US, 0},
	{0x3E205556, SuikodenTactics, EU, 0},
	{0xB808413B, SuikodenTactics, JP, 0},
	{0xA33AF77A, TenchuFS, US, 0},
	{0x64C58FB4, TenchuFS, US, 0},
	{0xE7CCCB1E, TenchuFS, EU, 0},
	{0x89E63B40, TenchuFS, RU, 0}, // Beta
	{0x1969B19A, TenchuFS, ES, 0}, // PAL Spanish
	{0xBF0DC4CE, TenchuFS, DE, 0},
	{0x696BBEC3, TenchuFS, KO, 0},
	{0x525C1994, TenchuFS, ASIA, 0},
	{0x0D73BBCD, TenchuFS, KO, 0},
	{0xAFBFB287, TenchuWoH, KO, 0},
	{0x767E383D, TenchuWoH, US, 0},
	{0x83261085, TenchuWoH, DE, 0}, // PAL German
	{0x7FA1510D, TenchuWoH, EU, 0}, // PAL ES, IT
	{0xC8DADF58, TenchuWoH, EU, 0},
	{0x13DD9957, TenchuWoH, JP, 0},
	{0x8BC95883, Sly3, US, 0},
	{0x3130A4D3, Sly3, US, 0}, // E3 Demo
	{0x35CCFA60, Sly3, US, 0}, // Regular Demo
	{0x8C146034, Sly3, EU, 0}, // Demo
	{0x8164C614, Sly3, EU, 0},
	{0xA8CC1583, Sly3, KO, 0},
	{0x518DD841, Sly2, KO, 0},
	{0x07652DD9, Sly2, US, 0},
	{0x5B93397F, Sly2, US, 0}, // E3 Demo
	{0xDD0B5E6C, Sly2, US, 0}, // Internal prototype disc
	{0x615EA2DB, Sly2, JP, 0}, // Kaitou Sly Cooper 2
	{0xFDA1CBF6, Sly2, EU, 0},
	{0x15DD1F6F, Sly2, NoRegion, 0},
	{0xA9C82AB9, DemonStone, US, 0},
	{0x7C7578F3, DemonStone, EU, 0},
	{0x22425C19, DemonStone, KO, 0},
	{0x506644B3, BigMuthaTruckers, EU, 0},
	{0x90F0D852, BigMuthaTruckers, US, 0},
	{0x5CC9BF81, TimeSplitters2, EU, 0},
	{0x12532F1C, TimeSplitters2, US, 0},
	{0xC818BEC2, LordOfTheRingsTwoTowers, US, 0},
	{0xDC43F2B8, LordOfTheRingsTwoTowers, EU, 0},
	{0x9ABF90FB, LordOfTheRingsTwoTowers, ES, 0},
	{0x5FF407EE, LordOfTheRingsTwoTowers, IT, 0},
	{0xC0E909E9, LordOfTheRingsTwoTowers, JP, 0},
	{0x6898435D, LordOfTheRingsTwoTowers, KO, 0},
	{0xDC2F9B98, LordOfTheRingsTwoTowers, CH, 0}, // cutie comment
	{0xEB198738, LordOfTheRingsThirdAge, US, 0},
	{0x614F4CF4, LordOfTheRingsThirdAge, EU, 0},
	{0x37CD4279, LordOfTheRingsThirdAge, KO, 0},
	{0xE169BAF8, RedDeadRevolver, US, 0},
	{0xE2E67E23, RedDeadRevolver, EU, 0},
	{0xF56C7948, HeavyMetalThunder, JP, 0},
	{0x6DF62AEA, BleachBladeBattlers, JP, 0},
	{0x6EB71AB0, BleachBladeBattlers, JP, 0}, // 2nd
	{0x3A446111, CastlevaniaCoD, US, 0},
	{0xF321BC38, CastlevaniaCoD, EU, 0},
	{0x950876FA, CastlevaniaCoD, KO, 0},
	{0x237B84D3, CastlevaniaCoD, CH, 0},
	{0x28270F7D, CastlevaniaLoI, US, 0},
	{0x306CDADA, CastlevaniaLoI, EU, 0},
	{0xA36CFF6C, CastlevaniaLoI, JP, 0},
	{0x9A93FE5D, CastlevaniaLoI, KO, 0},
	{0x7985D894, FinalFightStreetwise, US, 0},
	{0xED4BF0D3, FinalFightStreetwise, US, 0}, // cutie comment
	{0x0BA2B682, FinalFightStreetwise, RU, 0},
	{0x73C560BA, FinalFightStreetwise, EU, 0},
	{0xCBB87BF9, EvangelionJo, JP, 0}, // cutie comment
	{0xC5B75C7C, Oneechanbara2Special, JP, 0}, // cutie comment
	{0xC0659AD1, NarutimateAccel, JP, 0}, // cutie comment
	{0xF3D9DFBE, NarutimateAccel, JP, 0},
	{0x59739DDE, Naruto, JP, 0}, // cutie comment
	{0xF7786EE4, EternalPoison, JP, 0}, // cutie comment
	{0x2BE55519, EternalPoison, US, 0},
	{0xE01F57EC, LegoBatman, US, 0}, // cutie comment
	{0xE01F57ED, LegoBatman, EU, 0},
	{0xE0347841, XE3, JP, 0}, // cutie comment
	{0xA4E88698, XE3, CH, 0},
	{0x2088950A, XE3, US, 0},
	// DMC(1)? {0x79B8A95F, DevilMayCry3, US, 0},
	{0x7F3D692D, DevilMayCry3, CH, 0},
	// {0x1A85E924, DevilMayCry3, CH, 0}, // same CRC as {GodOfWar, NoRegion}
	{0xB1995E29, ShadowofRome, EU, 0}, // cutie comment
	{0x958DCA28, ShadowofRome, EU, 0},
	{0x57818AF6, ShadowofRome, US, 0},
	{0x1E210E60, ShadowofRome, US, 0}, // Demo
	{0xF21EE6E0, CrashNburn, US, 0},
	{0x54A548B4, CrashNburn, EU, 0},
	{0x694A998E, TombRaiderUnderworld, JP, 0}, // cutie comment
	{0x8E214549, TombRaiderUnderworld, EU, 0},
	{0x8E265148, TombRaiderUnderworld, RU, 0}, // Unofficial RU-version
	{0x618769D6, TombRaiderUnderworld, US, 0},
	{0xB639EB17, TombRaiderAnniversary, US, 0}, // Also needed for automatic mipmapping
	{0xB05805B6, TombRaiderAnniversary, JP, 0}, // cutie comment
	{0xA629A376, TombRaiderAnniversary, EU, 0},
	{0xBC8B3F50, TombRaiderLegend, US, 0}, // cutie comment
	{0x365172A0, TombRaiderLegend, JP, 0},
	{0x05177ECE, TombRaiderLegend, EU, 0},
	{0x5C891FF1, Black, US, 0},
	{0xCAA04879, Black, EU, 0},
	{0xADDFF505, Black, EU, 0},
	{0xB3A9F9ED, Black, JP, 0},
	{0x879CDA5E, StarWarsForceUnleashed, US, 0},
	{0x137C792E, StarWarsForceUnleashed, US, 0},
	{0xDAF2145C, StarWarsForceUnleashed, EU, 0},
	{0x503BF9E1, StarWarsBattlefront, NoRegion, 0}, // EU and US versions have the same CRC
	{0x02F4B541, StarWarsBattlefront2, NoRegion, 0}, // EU and US versions have the same CRC
	{0xA8DB29DF, BlackHawkDown, EU, 0},
	{0x25FC361B, DevilMayCry3, US, 0}, // SE
	{0x2F7D8AD5, DevilMayCry3, US, 0},
	{0x0BED0AF9, DevilMayCry3, US, 0},
	{0x18C9343F, DevilMayCry3, EU, 0}, // SE
	{0x7ADCB24A, DevilMayCry3, EU, 0},
	{0x79C952B0, DevilMayCry3, JP, 0}, // SE
	{0x7F3DDEAB, DevilMayCry3, JP, 0},
	{0x05931990, DevilMayCry3, KO, 0},
	{0x4AD36D59, DevilMayCry3, RU, 0},
	{0xBEBF8793, BurnoutTakedown, US, 0},
	{0x75BECC18, BurnoutTakedown, EU, 0},
	{0xCE49B0DE, BurnoutTakedown, EU, 0},
	{0x381EE9EF, BurnoutTakedown, EU, 0}, // E3 Demo
	{0xD224D348, BurnoutRevenge, US, 0},
	{0x7E83CC5B, BurnoutRevenge, EU, 0},
	{0x2CAC3DBC, BurnoutRevenge, EU, 0},
	{0xEEA60511, BurnoutRevenge, KO, 0},
	{0x8C9576A1, BurnoutDominator, US, 0},
	{0x8C9576B4, BurnoutDominator, EU, 0},
	{0x8C9C76B4, BurnoutDominator, EU, 0},
	{0x4A0E5B3A, MidnightClub3, US, 0}, // dub
	{0xEBE1972D, MidnightClub3, EU, 0}, // dub
	{0x60A42FF5, MidnightClub3, US, 0}, // remix
	{0xD03D4C77, SpyroNewBeginning, US, 0},
	{0x0EE5646B, SpyroNewBeginning, EU, 0},
	{0xB80CE8EC, SpyroEternalNight, US, 0},
	{0x8AE9536D, SpyroEternalNight, EU, 0},
	{0xC95F0198, SpyroEternalNight, NoRegion, 0},
	{0x43AB7214, TalesOfLegendia, US, 0},
	{0x1F8640E0, TalesOfLegendia, JP, 0},
	{0xE4F5DA2B, TalesOfLegendia, KO, 0},
	{0xA79B0491, NanoBreaker, JP, 0},
	{0x98C7B76D, NanoBreaker, US, 0},
	{0x7098BE76, NanoBreaker, KO, 0},
	{0x9B89F425, NanoBreaker, EU, 0},
	{0x519E816B, Kunoichi, US, 0}, // Nightshade
	{0x3FB419FD, Kunoichi, JP, 0},
	{0x086D198E, Kunoichi, CH, 0},
	{0x3B470BBD, Kunoichi, EU, 0},
	{0x6BA65DD8, Kunoichi, KO, 0},
	{0XD3F182A3, Yakuza, EU, 0},
	{0x6F9F99F8, Yakuza, EU, 0},
	{0x388F687B, Yakuza, US, 0},
	{0xC1B91FC5, Yakuza, US, 0}, // Demo
	{0xB7B3800A, Yakuza, JP, 0},
	{0xA60C2E65, Yakuza2, EU, 0},
	{0x800E3E5A, Yakuza2, EU, 0},
	{0x97E9C87E, Yakuza2, US, 0},
	{0xB1EBD841, Yakuza2, US, 0},
	{0xC6B95C48, Yakuza2, JP, 0},
	{0x9000252A, SkyGunner, JP, 0},
	{0x93092623, SkyGunner, JP, 0},
	{0xA9461CB2, SkyGunner, US, 0},
	{0xC71DE999, SkyGunner, US, 0}, // Regular Demo
	{0xAADF3287, SkyGunner, US, 0}, // Trade Demo
	{0xB799A60C, SkyGunner, NoRegion, 0},
	{0x90F4B057, ZettaiZetsumeiToshi2, CH, 0},
	{0xC988ECBB, ZettaiZetsumeiToshi2, JP, 0},
	{0x2905C5C6, ZettaiZetsumeiToshi2, US, 0}, // Raw Danger!
	{0xBD17248E, ShinOnimusha, JP, 0},
	{0xBE17248E, ShinOnimusha, JP, 0},
	{0xB817248E, ShinOnimusha, JP, 0},
	{0x812C5A96, ShinOnimusha, EU, 0},
	{0xFE44479E, ShinOnimusha, US, 0},
	{0xFFDE85E9, ShinOnimusha, US, 0},
	{0xE21404E2, GetaWay, US, 0},
	{0x458485EF, GetaWay, EU, 0},
	{0xE78971DF, GetaWayBlackMonday, US, 0},
	{0x342D97FA, GetaWayBlackMonday, US, 0}, // Demo
	{0xE8C0AD1A, GetaWayBlackMonday, JP, 0},
	{0x09C3DF79, GetaWayBlackMonday, EU, 0},
	{0x1130BF23, SakuraTaisen, CH, 0}, // cutie comment
	{0x4FAE8B83, SakuraTaisen, KO, 0},
	{0xEF06DBD6, SakuraWarsSoLongMyLove, JP, 0}, // cutie comment
	{0xDD41054D, SakuraWarsSoLongMyLove, US, 0}, // cutie comment
	{0xC2E3A7A4, SakuraWarsSoLongMyLove, KO, 0},
	{0x4A4B623A, FightingBeautyWulong, JP,0}, // cutie comment
	{0x5AC7E79C, TouristTrophy, CH, 0}, // cutie comment
	{0xFF9C0E93, TouristTrophy, US, 0},
	{0xCA9AA903, TouristTrophy, EU, 0},
	{0xA1B3F232, GTASanAndreas, EU, 0}, // cutie comment
	{0xB440A8FE, GTASanAndreas, EU, 0},
	{0x399A49CA, GTASanAndreas, US, 0},
	{0x2C6BE434, GTASanAndreas, US, 0},
	{0x60FE139C, GTASanAndreas, JP, 0},
	{0x2615F542, FrontMission5, JP, 0},
	{0xF60255AC, FrontMission5, JP, 0},
	{0xCB783836, FrontMission5, JP, 0},
	{0xB7532DF6, FrontMission5, JP, 0},
	{0xAEDAEE99, GodHand, JP, 0},
	{0x6FB69282, GodHand, US, 0},
	{0x924C4AA6, GodHand, KO, 0},
	{0xDE9722A5, GodHand, EU, 0},
	{0x9637D496, KnightsOfTheTemple2, JP, 0}, // cutie comment
	{0x4E811100, UltramanFightingEvolution, JP, 0}, // cutie comment
	{0xF7F181C3, DeathByDegreesTekkenNinaWilliams, CH, 0}, // cutie comment
	{0xF088FA5B, DeathByDegreesTekkenNinaWilliams, KO, 0},
	{0x59683BB0, DeathByDegreesTekkenNinaWilliams, EU, 0},
	{0x5B659BED, Grandia3, JP, 0},
	{0x5B657DAD, Grandia3, US, 0},
	{0x830B6FB1, TalesofSymphonia, JP, 0},
	{0x86C57952, SoulCalibur2, JP, 0},
	{0x83AFB38A, SoulCalibur2, KO, 0},
	{0xE1B01308, SoulCalibur2, US, 0},
	{0x4B66F38C, SoulCalibur2, US, 0}, // Demo
	{0x632A5116, SoulCalibur2, EU, 0},
	{0xFB8554A0, SoulCalibur3, JP, 0},
	{0x7C7B9E71, SoulCalibur3, JP, 0},
	{0x027C604C, SoulCalibur3, US, 0},
	{0x24090A12, SoulCalibur3, EU, 0},
	{0x3BA95B70, SoulCalibur3, EU, 0},
	{0xBC5480A3, SoulCalibur3, EU, 0},
	{0xBE40779A, SoulCalibur3, RU, 0},
	{0x37B99B14, SoulCalibur3, KO, 0},
	{0xFC0F8A5B, Simple2000Vol114, JP, 0},
	{0xBDD9BAAD, UrbanReign, US, 0}, // cutie comment
	{0x0418486E, UrbanReign, RU, 0},
	{0xAE4BEBD3, UrbanReign, EU, 0},
	{0x48AC09BC, SteambotChronicles, EU, 0},
	{0x9F391882, SteambotChronicles, US, 0},
	{0xFEFCF9DE, SteambotChronicles, JP, 0}, // Ponkotsu Roman Daikatsugeki: Bumpy Trot 
	{0XE1BF5DCA, SuperManReturns, US, 0},
	{0XE8F7BAB6, SuperManReturns, EU, 0},
	{0x06A7506A, SacredBlaze, JP, 0},
	{0x4CE7FB04, ItadakiStreet, JP, 0},
	{0x2479F4A9, Jak2, EU, 0},
	{0x9184AAF1, Jak2, US, 0},
	{0x12804727, Jak3, EU, 0},
	{0x644CFD03, Jak3, US, 0},
	{0xDF659E77, JakX, EU, 0},
	{0x3091E6FB, JakX, US, 0},
	{0x4653CA3E, HarleyDavidson, US, 0},
	// Games list for Automatic Mipmapping
	// Basic mipmapping
	{0x194C9F38, AceCombatZero, EU, 0},
	{0x65729657, AceCombatZero, US, 0},
	{0xA04B52DB, AceCombatZero, JP, 0},
	{0x09B3AD4D, ApeEscape2, EU, 0},
	{0xBDD9F5E1, ApeEscape2, US, 0},
	{0xFE0A6AB6, ApeEscape2, JP, 0}, // Saru! Get You! 2
	{0x0940508D, BrianLaraInternationalCricket, EU, 0},
	{0x0BAA8DD8, DarkCloud, EU, 0},
	{0x1DF41F33, DarkCloud, US, 0},
	{0xA5C05C78, DarkCloud, US, 0},
	{0x60AA5049, DarkCloud, KO, 0},
	{0xECD8E386, DarkCloud, JP, 0},
	{0x67A29886, DestroyAllHumans, US, 0},
	{0xE3E8E893, DestroyAllHumans, EU, 0},
	{0x42DF8C8C, DestroyAllHumans2, US, 0},
	{0x743E10C2, DestroyAllHumans2, EU, 0},
	{0x67C38BAA, FIFA03, US, 0},
	{0x722BBD62, FIFA03, EU, 0},
	{0x2BCCF704, FIFA03, EU, 0},
	{0xCC6AA742, FIFA04, KO, 0},
	{0x2C6A4E2E, FIFA04, US, 0},
	{0x684ADFC6, FIFA04, EU, 0},
	{0x972611BB, FIFA05, US, 0},
	{0x972719A3, FIFA05, EU, 0},
	{0xC5473413, HarryPotterATCOS, NoRegion, 0}, // EU and US versions have the same CRC - Chamber Of Secrets
	{0xE90BE9F8, HarryPotterATCOS, JP, 0 }, // Coca Cola original Version
	{0x51E019BC, HarryPotterATPOA, NoRegion, 0 }, // EU and US versions have the same CRC - Prisoner of Azkaban
	{0x99A8B4FF, HarryPotterATPOA, KO, 0 },
	{0xA8901AD6, HarryPotterATPOA, JP, 0 }, // Harry Potter to Azkaban no Shuujin
	{0x4C01B1B0, HarryPotterOOTP, US, 0}, // Order Of The Phoenix
	{0x01A9BF0E, HarryPotterOOTP, EU, 0},
	{0x230CB71D, SoulReaver2, US, 0},
	{0x6F991F52, SoulReaver2, JP, 0},
	{0x6D8B4CD1, SoulReaver2, EU, 0},
	{0x60F56E8F, SoulReaver2, RU, 0}, // Unofficial RU-version
	{0x728AB07C, LegacyOfKainDefiance, US, 0},
	{0xBCAD1E8A, LegacyOfKainDefiance, EU, 0},
	{0x018AC37C, LegacyOfKainDefiance, RU, 0}, // Unofficial RU-version
	{0x28D09BF9, NicktoonsUnite, US, 0},
	{0xF25266C4, NicktoonsUnite, EU, 0}, // Nickelodeon SpongeBob SquarePants And Friends Unite
	{0xCE4933D0, RatchetAndClank, US, 0},
	{0x6F191506, RatchetAndClank, US, 0}, // E3 Demo
	{0x81CBFEA2, RatchetAndClank, US, 0}, // EB Games Demo
	{0x56A35F77, RatchetAndClank, JP, 0},
	{0x76F724A3, RatchetAndClank, EU, 0},
	{0xB3A71D10, RatchetAndClank2, US, 0}, // Going Commando
	{0x38996035, RatchetAndClank2, US, 0},
	{0xDF6F94A1, RatchetAndClank2, US, 0}, // Demo - Going Commando & Jak II
	{0x8CAA5F16, RatchetAndClank2, JP, 0}, // Gagaga! Ginga no Commando-ssu
	{0x2F486E6F, RatchetAndClank2, EU, 0},
	{0x45FE0CC4, RatchetAndClank3, US, 0}, // Up Your Arsenal
	{0x2A12175A, RatchetAndClank3, US, 0}, // Regular Demo
	{0x64DC6000, RatchetAndClank3, JP, 0}, // Totsugeki! Galactic Rangers
	{0x17125698, RatchetAndClank3, EU, 0},
	{0x9BFBCD42, RatchetAndClank4, US, 0}, // Deadlocked
	{0x2EC9DA96, RatchetAndClank4, JP, 0}, // GiriGiri Ginga no Giga Battle
	{0xD697D204, RatchetAndClank4, EU, 0}, // Ratchet Gladiator
	{0x8661F7BA, RatchetAndClank5, US, 0}, // Size Matters
	{0xFCB981D5, RatchetAndClank5, EU, 0}, // Size Matters
	{0x8634861F, RickyPontingInternationalCricket, EU, 0},
	{0xA56A0525, Quake3Revolution, US, 0},
	{0x2064ACE6, Quake3Revolution, EU, 0},
	{0xDDAC3815, Shox, US, 0},
	{0x78FFA39F, Shox, EU, 0},
	{0x3DF10389, Shox, EU, 0},
	{0xF17AF8BD, TheIncredibleHulkUD, US, 0},
	{0x6B3D50A5, TheIncredibleHulkUD, EU, 0},
	{0x2B58234D, TribesAerialAssault, US, 0},
	{0x4D22DB95, Whiplash, US, 0},
	{0xB1BE3E51, Whiplash, EU, 0},
};

std::map<uint32, CRC::Game*> CRC::m_map;

std::string ToLower( std::string str )
{
	transform( str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

// The exclusions list is a comma separated list of: the word "all" and/or CRCs in standard hex notation (0x and 8 digits with leading 0's if required).
// The list is case insensitive and order insensitive.
// E.g. Disable all CRC hacks:          CrcHacksExclusions=all
// E.g. Disable hacks for these CRCs:   CrcHacksExclusions=0x0F0C4A9C, 0x0EE5646B, 0x7ACF7E03
bool IsCrcExcluded(std::string exclusionList, uint32 crc)
{
	std::string target = format("0x%08x", crc);
	exclusionList = ToLower(exclusionList);
	return exclusionList.find(target) != std::string::npos || exclusionList.find("all") != std::string::npos;
}

CRC::Game CRC::Lookup(uint32 crc)
{
	printf("GSdx Lookup CRC:%X\n", crc);
	if(m_map.empty())
	{
		std::string exclusions = theApp.GetConfigS("CrcHacksExclusions");
		if (exclusions.length() != 0)
			printf( "GSdx: CrcHacksExclusions: %s\n", exclusions.c_str() );

		int crcDups = 0;
		for(size_t i = 0; i < countof(m_games); i++)
		{
			if( !IsCrcExcluded( exclusions, m_games[i].crc ) ){
				if(m_map[m_games[i].crc]){
					printf("[FIXME] GSdx: Duplicate CRC: 0x%x: (game-id/region-id) %d/%d overrides %d/%d\n"
						, m_games[i].crc, m_games[i].title, m_games[i].region, m_map[m_games[i].crc]->title, m_map[m_games[i].crc]->region);
					crcDups++;
				}

				m_map[m_games[i].crc] = &m_games[i];
			}
			//else
			//	printf( "GSdx: excluding CRC hack for 0x%08x\n", m_games[i].crc );
		}
		if(crcDups)
			printf("[FIXME] GSdx: Duplicate CRC: Overall: %d\n", crcDups);
	}

	auto i = m_map.find(crc);

	if(i != m_map.end())
	{
		return *i->second;
	}

	return m_games[0];
}
