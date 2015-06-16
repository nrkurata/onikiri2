// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Ryota Shioya.
// Copyright (c) 2005-2015 Masahiro Goshima.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
// 
// 


#include <pch.h>

#include "Sim/Predictor/LatPred/LatPred.h"
#include "Sim/Predictor/HitMissPred/CounterBasedHitMissPred.h"
#include "Sim/Predictor/HitMissPred/StaticHitMissPred.h"
#include "Sim/ExecUnit/ExecUnitIF.h"
#include "Sim/Pipeline/Scheduler/Scheduler.h"
#include "Sim/Op/Op.h"
#include "Sim/Core/Core.h"

using namespace Onikiri;

namespace Onikiri
{
    HookPoint<LatPred> LatPred::s_latencyPredictionHook;
}; // Onikiri

LatPred::LatPred() :
    m_execLatencyInfo(0),
    m_hmPredictor(0),
    m_core(0),
    m_numHit(0),
    m_numMiss(0),
    m_numLoadHitPredToHit(0),
    m_numLoadHitPredToMiss(0),
    m_numLoadMissPredToHit(0),
    m_numLoadMissPredToMiss(0)
{
}

LatPred::~LatPred()
{
    ReleaseParam();
}

void LatPred::Initialize(InitPhase phase)
{
    if(phase == INIT_PRE_CONNECTION){
        LoadParam();
    }
    else if(phase == INIT_POST_CONNECTION){
        CheckNodeInitialized( "execLatencyInfo", m_execLatencyInfo );
        CheckNodeInitialized( "hmPredictor", m_hmPredictor );
    }
}

// ���C�e���V�\��
// ����2���x���iL1/L2�j�̗\���ɂ̂ݑΉ�
// L1�~�X��́C���L2�q�b�g�Ɨ\�������
void LatPred::Predict(OpIterator op)
{
    const OpClass& opClass = op->GetOpClass();

    LatPredResult result;

    s_latencyPredictionHook.Trigger(op, this, HookType::HOOK_BEFORE);

    if( !s_latencyPredictionHook.HasAround() ) {
        // �{���̏���
        // Load/store hit miss prediction
        // ���C�e���V�\���ƃX�P�W���[�����O�̃��f���ɂ��Ă͂��̃t�@�C���̖������Q��
        ExecUnitIF* execUnit = op->GetExecUnit();
        int latencyCount = execUnit->GetLatencyCount(opClass);
        
        if( opClass.IsLoad() ){

            static const int MAX_PRED_LEVEL = LatPredResult::MAX_COUNT;
            ASSERT( latencyCount <= MAX_PRED_LEVEL, "Make MAX_PRED_LEVEL large enough." );
            result.SetCount( latencyCount ); 

            // Get latencies
            int latencies[ MAX_PRED_LEVEL ];
            for( int i = 0; i < latencyCount; ++i ){
                latencies[i] = execUnit->GetLatency( opClass, i );
            }
            
            // L1 prediction
            bool lv1Hit = m_hmPredictor->Predict( op );
            result.Set( 0, latencies[0], lv1Hit );
            
            // Issue latency
            int issueLatency = op->GetScheduler()->GetIssueLatency();

            // Lv2 & higher caches are predicted as always hit.
            bool prevLevelWokeup = lv1Hit;
            for( int i = 1; i < latencyCount; ++i ){

                // 0  1  2  3  4  5  6  7  8                  
                // SC IS IS IS IS L1 L2 L2 L2
                //             SC IS IS IS IS EX
                //                ^^ L1 miss detected point
                //                   SC IS IS IS IS EX
                // detected     : 1(SC) + 4(IS) + 1(L1)
                // second issue: 1(SC) + L2(3)
                // second issue - detected = L2 - IS - L1

                int wakeupTime = latencies[i];
                if( prevLevelWokeup ){
                    int detectTime = issueLatency + latencies[i - 1] + 1 + 1;// + rescheduling + select
                    if( wakeupTime < detectTime ){
                        wakeupTime = detectTime;
                    }
                }
                result.Set( i, wakeupTime, true );
                prevLevelWokeup = true;
            }

            // A last level memory is predicted as always miss and does not wakeup.
            if( latencyCount > 1 ){
                result.Set( 
                    latencyCount - 1, 
                    latencies[latencyCount - 1], 
                    false 
                );
            }


        }   // if(opClass.IsLoad()) {
        else{
            
            ASSERT( latencyCount == 1, "A variable latency is not supported except case of Load." );

            int latency = execUnit->GetLatency( opClass, 0 );

            result.SetCount( 1 );
            result.Set( 0, latency, true );
        }

        op->SetLatPredRsult( result );
    }
    else {
        s_latencyPredictionHook.Trigger(op, this, HookType::HOOK_AROUND);
    }

    s_latencyPredictionHook.Trigger(op, this, HookType::HOOK_AFTER);

}

//
// predictionHit : ���C�e���V�\���̌��ʂ��I���������ǂ���(L1�q�b�g�̈Ӗ��ł͂Ȃ�)
//
void LatPred::Commit( OpIterator op )
{
    if( !op->GetOpClass().IsLoad() ){
        return;
    }

    int latency = op->GetIssueState().executionLatency;
    const LatPredResult& latPredResult = 
        op->GetLatPredRsult();
    const LatPredResult::Scheduling& predSched = latPredResult.Get(0);
    
    bool predictedAsHitLv1 = predSched.wakeup;
    bool hitMissOfLV1      = latency == predSched.latency ? true : false;
    if( hitMissOfLV1 ){
        if( predictedAsHitLv1 ){
            m_numLoadHitPredToHit++;
        } 
        else{
            m_numLoadHitPredToMiss++;
        }
    }
    else{
        if( predictedAsHitLv1 ){
            m_numLoadMissPredToHit++;
        } else {
            m_numLoadMissPredToMiss++;
        }
    }

    if( predictedAsHitLv1 == hitMissOfLV1 )
        m_numHit++;
    else
        m_numMiss++;

    m_hmPredictor->Commit( op, hitMissOfLV1 );
}

void LatPred::Finished( OpIterator op )
{
    if( !op->GetOpClass().IsLoad() ){
        return;
    }

    int latency = op->GetIssueState().executionLatency;
    const LatPredResult::Scheduling& predSched = op->GetLatPredRsult().Get(0);
    bool hitMissOfLV1      = latency == predSched.latency ? true : false;
    m_hmPredictor->Finished( op, hitMissOfLV1 );
}

/*

--- ���C�e���V�\���ƃX�P�W���[�����O�̃��f��

���C�e���V�\�����s������(Ip:producer)�ƁC�\�����s�������߂�
�ˑ����閽��(Ic:consumer)���ǂ̂悤�ɃX�P�W���[�����邩�D

��F
    Ip: load r1 = [A]
    Ic: add  r2 = r1 + 4

���CIp ���L���b�V���Ƀ~�X����Ɨ\�������ꍇ���l����
�����̘_���ł�Ip ��Ic �͈ȉ��̂悤�ɃX�P�W���[�������Ƃ��Ă���

    time: 0  1  2  3  4  5  6  7  8
    --------------------------------
    Ip:   S  I  R  1  2  2  2  W
    Ic:   S  S  S  S  S  I  R  X  W

    S : Scheduling
    R : Register Read
    1 : L1 access
    2 : L2 access
    X : execute
    W : Register Write

    Issue latency : 1cycle
    L1 latency    : 1cycle
    L2 latency    : 3cycle


���C�e���V�\�����s�������߂������s�I�����邩�i�q�b�g/�~�X�̔����j��
��{�I�Ɏ��s�I�����܂ŕs���ł���D
Ip ��6cycle �ڈȍ~�̃t�H���[�f�B���O�|�[�g�A7cycle �ڂ̃��W�X�^
�������݃|�[�g�𗘗p���邽�߂ɂ́C�����ƃ|�[�g��\��
���Ă����Ȃ��Ă͂Ȃ�Ȃ����A���̂悤�Ȏ����͑Ó��Ƃ͍l���ɂ����D
�i�|�[�g��\�񂵂Ă����Ȃ��ƁC���̎����s����Ă������̖��߂Ǝ����������N�����D

���ۂ̃n�[�h�E�F�A�ł́C�ȉ��̂悤�Ƀ~�X����Ɨ\�������ꍇ�ɂ�
Ip ��2�񔭍s���s�������ɂȂ��Ă���ƍl������D
�ȉ��̗�ł́CIp0 �Ń~�X����������3�T�C�N���ڈȍ~��L2 �̃A�N�Z�X���J�n���C
���傤��L1 �Ƀf�[�^�����Ă���^�C�~���O��2��ڂ�Ip1 �𔭍s����D
Ic �́CIp1 �̎��s��ɂ��킹��`�Ŕ��s���s���D

    time: 0  1  2  3  4  5  6  7  8  9
    -----------------------------------
    Ip0:  S  I  R  1 (2  2  2) 
    Ip1:  .  .  .  .  S  I  R  1  W
    Ic:   S  S  S  S  S  S  I  R  X  W

*/
