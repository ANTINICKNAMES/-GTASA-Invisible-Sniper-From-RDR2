#include <plugin.h> // Plugin-SDK version 1002 from 2025-12-09 23:18:09

#include <CBulletInfo.h>
#include <CBulletTraces.h>
#include <CCamera.h>
#include <CGameLogic.h>
#include <CPedIK.h>
#include <CStats.h>
#include <CWorld.h>

#ifdef DEBUG
#include <CLines.h>
#include <CMessages.h>
#endif // DEBUG

uint32_t LastTimePlayerWasShot = 0;

using namespace plugin;

struct Main
{
    Main()
    {
        /*Events::gameProcessEvent += [] {
            gInstance.CheckIfPlayerInForbiddenZone(); 
            };*/

        patch::ReplaceFunction(0x441770, CheckIfPlayerInForbiddenZone);

        Events::reInitGameEvent += [] { LastTimePlayerWasShot = CTimer::m_snTimeInMilliseconds; };

#ifdef DEBUG
        Events::renderEffectsEvent += [] {

            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)true);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);

            const auto  ped = FindPlayerPed();
            const auto& coords = ped->GetPosition();

            const auto cam = &TheCamera.m_aCams[TheCamera.m_nActiveCam];
            CVector camOriginPos = cam->m_vecSource;
            RwV3d playerHeadPos;

            RpHAnimHierarchy* hier = GetAnimHierarchyFromSkinClump(FindPlayerPed()->m_pRwClump);
            int idx = RpHAnimIDGetIndex(hier, FindPlayerPed()->m_apBones[2]->m_nNodeId);
            RwMatrix* mats = RpHAnimHierarchyGetMatrixArray(hier);
            playerHeadPos = mats[idx].pos;

            CLines::RenderLineWithClipping(camOriginPos.x, camOriginPos.y, camOriginPos.z, playerHeadPos.x, playerHeadPos.y, playerHeadPos.z, 0xFF0000FF, 0xFF0000FF);

            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)5);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)6);
            RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)false);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
            };
#endif // DEBUG

    }

    static void __cdecl CheckIfPlayerInForbiddenZone(bool bAlways)
    {
        CPed* ped = FindPlayerPed();

        if (!ped || !ped->IsAlive())
            return;

        CVector coords = ped->GetPosition();

        //if ((CTimer::m_FrameCounter % 32) != 18 || coords.z > 950.0f)
        if (LastTimePlayerWasShot + 1000 > CTimer::m_snTimeInMilliseconds || coords.z > 950.0f)
            return;

        // 0x8A5EC8; Las Venturas
        CVector town1ShapeVertices[] = {
            {3000.0f, 535.0f, 0.0f},
            {1759.0f, 576.0f, 0.0f},
            {989.0f, 693.0f, 0.0f},
            {-128.0f, 490.0f, 0.0f},
            {-845.0f, 707.0f, 0.0f},
            {-1477.0f, 1677.0f, 0.0f},
            {-2154.0f, 2497.0f, 0.0f},
            {-2971.0f, 2180.0f, 0.0f},
            {-3000.0f, 3000.0f, 0.0f},
            {3000.0f, 3000.0f, 0.0f}
        };

        // 0x8A5E58; San Fierro
        CVector town2ShapeVertices[] = {
            {28.0f, -3000.0f, 0.0f},
            {-30.0f, -1280.0f, 0.0f},
            {-148.0f, -911.0f, 0.0f},
            {-487.0f, -372.0f, 0.0f},
            {-1028.0f, -424.0f, 0.0f},
            {-1145.0f, 479.0f, 0.0f},
            {-1461.0f, 1488.0f, 0.0f},
            {-3000.0f, 1668.0f, 0.0f},
            {-3000.0f, -3000.0f, 0.0f}
        };

        // LV
        if (CStats::GetStatValue(STAT_CITY_UNLOCKED) <= 1.0f) {
            if (CGameLogic::IsPointWithinLineArea(town1ShapeVertices, 10, coords)) 
            {
                gInstance.ShootPlayerInForbiddenZone(ped);
            }
        }

        // SF
        if (CStats::GetStatValue(STAT_CITY_UNLOCKED) <= 0.0f) {
            if (CGameLogic::IsPointWithinLineArea(town2ShapeVertices, 9, coords)) 
            {
                gInstance.ShootPlayerInForbiddenZone(ped);
            }
        }

        LastTimePlayerWasShot = CTimer::m_snTimeInMilliseconds;
    }

    void ShootPlayerInForbiddenZone(CPed* pPed)
    {
        CVector pPlayerPos = pPed->GetPosition();
        
        RwV3d playerHeadPos;
        RpHAnimHierarchy* hier = GetAnimHierarchyFromSkinClump(pPed->m_pRwClump);
        int idx = RpHAnimIDGetIndex(hier, pPed->m_apBones[2]->m_nNodeId);
        RwMatrix* mats = RpHAnimHierarchyGetMatrixArray(hier);
        playerHeadPos = mats[idx].pos;

        pPlayerPos = playerHeadPos;


        CVector pPosn = TheCamera.m_aCams[TheCamera.m_nActiveCam].m_vecSource;
        CVector velocity = pPlayerPos;
        velocity -= pPosn;
        velocity.Normalise();

        velocity *= 16.f;

        int i;
        for (i = 0; i < MAX_BULLET_INFOS; i++) {
            if (!aBulletInfos[i].m_bExists)
                break;
        }

        if (i == MAX_BULLET_INFOS)
            return;

        aBulletInfos[i].m_pCreator = nullptr;
        aBulletInfos[i].m_nWeaponType = WEAPONTYPE_SNIPERRIFLE;
        aBulletInfos[i].m_nDamage = 500; // CWeaponInfo::GetWeaponInfo(weaponType, eWeaponSkill::STD)->m_nDamage;
        aBulletInfos[i].m_vecPosition = pPosn;
        aBulletInfos[i].m_vecVelocity = velocity;
        aBulletInfos[i].m_nDestroyTime = (float)(CTimer::m_snTimeInMilliseconds);
        aBulletInfos[i].m_bExists = true;

        pPosn.z -= 0.1f;

        CColPoint shotCP;
        CEntity* shotHitEntity;
        
        if (CWorld::ProcessLineOfSight(pPosn, playerHeadPos, shotCP, shotHitEntity, true, true, true, true, true, false, false, true))
            CBulletTraces::AddTrace(&pPosn, &shotCP.m_vecPoint, 0.01f, 500, 255);
        else
            CBulletTraces::AddTrace(&pPosn, (CVector*)&playerHeadPos, 0.01f, 500, 255);

        //CBulletTraces::AddTrace(&pPosn, (CVector*)&playerHeadPos, WEAPONTYPE_M4, nullptr);
    }
} gInstance;
