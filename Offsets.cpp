#include "Offsets.h"
#include <iostream>

Offsets g_Offsets;

Offsets::Offsets() {
}

Offsets::~Offsets() {
}

bool Offsets::Initialize() {
    // Manually setting offsets that are known from the previously loaded file
    // We're hardcoding these instead of loading from JSON to avoid dependencies

    // client.dll offsets
    this->EntityList = 0x17CE6A0;
    this->Matrix = 0x1820FE0;
    this->ViewAngle = 0x1891500;
    this->LocalPlayerController = 0x1810FF0;
    this->LocalPlayerPawn = 0x16F2520;
    this->GlobalVars = 0x16A9AF8;
    this->PlantedC4 = 0x17D6680;
    this->Sensitivity = 0x1891388;

    // inputsystem.dll offsets
    this->InputSystem = 0x35720;

    // Button offsets
    this->Buttons.Attack = 0x141F4E8;
    this->Buttons.Jump = 0x1420A80;
    this->Buttons.Right = 0x1420638;
    this->Buttons.Left = 0x1420628;

    // Entity offsets
    this->Entity.IsAlive = 0x7EC;
    this->Entity.PlayerPawn = 0x7EC;
    this->Entity.iszPlayerName = 0x640;

    // Pawn offsets
    this->Pawn.BulletServices = 0x16D8;
    this->Pawn.CameraServices = 0x10F0;
    this->Pawn.pClippingWeapon = 0x12B0;
    this->Pawn.isScoped = 0x13E4;
    this->Pawn.isDefusing = 0x13F0;
    this->Pawn.TotalHit = 0x40;
    this->Pawn.Pos = 0x12AC;
    this->Pawn.CurrentArmor = 0x14D8;
    this->Pawn.MaxHealth = 0x328;
    this->Pawn.CurrentHealth = 0x32C;
    this->Pawn.GameSceneNode = 0x310;
    this->Pawn.BoneArray = 0x1F0;
    this->Pawn.angEyeAngles = 0x1508;
    this->Pawn.vecLastClipCameraPos = 0x1280;
    this->Pawn.iShotsFired = 0x1404;
    this->Pawn.flFlashDuration = 0x14C4;
    this->Pawn.aimPunchAngle = 0x1718;
    this->Pawn.aimPunchCache = 0x1728;
    this->Pawn.iIDEntIndex = 0x15B4;
    this->Pawn.iTeamNum = 0x3BF;
    this->Pawn.iFovStart = 0x214;
    this->Pawn.fFlags = 0x3C8;
    this->Pawn.bSpottedByMask = 0x1628 + 0x8;
    this->Pawn.AbsVelocity = 0x3B8;
    this->Pawn.m_bWaitForNoAttack = 0x13D0;

    // Global var offsets
    this->GlobalVar.RealTime = 0x00;
    this->GlobalVar.FrameCount = 0x04;
    this->GlobalVar.MaxClients = 0x10;
    this->GlobalVar.IntervalPerTick = 0x14;
    this->GlobalVar.CurrentTime = 0x2C;
    this->GlobalVar.CurrentTime2 = 0x30;
    this->GlobalVar.TickCount = 0x40;
    this->GlobalVar.IntervalPerTick2 = 0x44;
    this->GlobalVar.CurrentNetchan = 0x0048;
    this->GlobalVar.CurrentMap = 0x0180;
    this->GlobalVar.CurrentMapName = 0x0188;

    // Player controller offsets
    this->PlayerController.m_steamID = 0x1118;
    this->PlayerController.m_hPawn = 0x60C;
    this->PlayerController.m_pObserverServices = 0x10F8;
    this->PlayerController.m_hObserverTarget = 0x44;
    this->PlayerController.m_hController = 0x123C;
    this->PlayerController.PawnArmor = 0xDDC;
    this->PlayerController.HasDefuser = 0xDF0;
    this->PlayerController.HasHelmet = 0xDEC;

    // EconEntity offsets
    this->EconEntity.AttributeManager = 0x10D0;

    // Weapon base data offsets
    this->WeaponBaseData.WeaponDataPTR = 0x360 + 0x08;
    this->WeaponBaseData.szName = 0x20;
    this->WeaponBaseData.Clip1 = 0x1570;
    this->WeaponBaseData.MaxClip = 0x80;
    this->WeaponBaseData.Item = 0x08;
    this->WeaponBaseData.ItemDefinitionIndex = 0x1BA;

    // C4 offsets
    this->C4.m_bBeingDefused = 0xEBC;
    this->C4.m_flDefuseCountDown = 0xED0;
    this->C4.m_nBombSite = 0xE80;

    std::cout << "CS2 Offsets initialized successfully." << std::endl;
    return true;
}