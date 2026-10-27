# 요구사항 재검수

요청 목록은 채팅 6개, PlayerState 9개, Vehicle 19개, GameMode 13개로 총 47개입니다.
아래 표의 확인은 코드/애셋 연결을 대조한 결과입니다. 모든 항목을 각각 수동 플레이로 검사했다는 뜻은 아닙니다.

## 채팅 — 6개

| 번호 | 요구사항 | 구현 근거 | 판정 |
|---|---|---|---|
| C1 | 위젯 클래스 | UChatUI | 구현 확인 |
| C2 | 위젯 블루프린트 애셋 | /Game/UI/WBP_Chat | 구현 확인 |
| C3 | 위젯 생성·연결 | Controller BeginPlay, UCXUI NativeConstruct에서 기존 WBP_Chat 타입 검색 | 구현 확인 |
| C4 | Enter 이벤트 | OnTextCommitted → OnChatInputTextCommitted | 구현 확인 |
| C5 | 메시지 화면 표시 | MultiLineEditableTextBox_ChatBox와 ChatScrollBox | 구현 확인 |
| C6 | 채팅 동기화 | CTSChatMessage 서버 RPC, STCChatMessage 클라이언트 RPC | 구현 확인 |

## PlayerState — 9개

| 번호 | 요구사항 | 구현 근거 | 판정 |
|---|---|---|---|
| P1 | Coins 속성·복제 | ACXPlayerState, UPROPERTY(Replicated) | 구현 확인 |
| P2 | 원화 증가·차감 함수 | AddCoins, TryRemoveCoins | 구현 확인 |
| P3 | 원화 서버 권한 검사 | HasAuthority 및 양수/잔고 검사 | 구현 확인 |
| P4 | Coins OwnerOnly | DOREPLIFETIME_CONDITION(ACXPlayerState, Coins, COND_OwnerOnly) | 구현 확인 |
| P5 | BonusMaxSpeed 속성·복제 | ACXPlayerState, UPROPERTY(Replicated) | 구현 확인 |
| P6 | 추가 최대 속도 증가 | AddBonusMaxSpeed | 구현 확인 |
| P7 | 속도 보너스 서버 권한 검사 | HasAuthority, 유한값·양수 검사 | 구현 확인 |
| P8 | BonusMaxSpeed OwnerOnly | DOREPLIFETIME_CONDITION(..., BonusMaxSpeed, COND_OwnerOnly) | 구현 확인 |
| P9 | 자신의 코인·보너스 UI | UCXMatchHUD에서 GetOwningPlayer의 PlayerState만 표시 | 구현 확인 |

## Vehicle — 19개

| 번호 | 요구사항 | 구현 근거 | 판정 |
|---|---|---|---|
| V1 | 좌클릭 InputAction 애셋·연결 | IA_Attack, 양 차량 BP에 지정, AttackMapping에 LeftMouseButton | 구현 확인 |
| V2 | 좌클릭 Server RPC | Attack → ServerAttack | 구현 확인 |
| V3 | 앞 범퍼 중앙 라인트레이스 | 메시 바운드 전면 중앙에서 LineTraceSingleByChannel | 구현 확인 |
| V4 | 상대 차량 ApplyDamage | ACXPawn 판별 후 UGameplayStatics::ApplyDamage | 구현 확인 |
| V5 | 가해자 +5, 피해자 -5 | TakeDamage에서 서버가 차감 성공 후 지급 | 구현 확인 |
| V6 | 5원 미만이면 이전 없음 | TryRemoveCoins(5) 실패 시 지급하지 않음 | 구현 확인 |
| V7 | 디버그 선·피격 파티클 | MulticastAttackEffects, DrawDebugLine, SpawnEmitterAtLocation | 구현 확인 |
| V8 | 코인 액터·오버랩 +10원 | ACXCoin, OnOverlap → AddCoins(10) | 구현 확인 |
| V9 | 코인 NetUpdateFrequency=1 | ACXPickup 생성자의 SetNetUpdateFrequency(1.f) | 구현 확인 |
| V10 | 획득 코인 파괴 | 보상 후 숨김/충돌 해제, SetLifeSpan(.25f) | 구현 확인 |
| V11 | 코인 파괴 시 파티클·사운드 | Destroyed → PlayCollectionEffects | 이번 검수에서 시점 수정 |
| V12 | 속도 버프 액터·오버랩 +10 | ACXSpeedBuff → AddBonusMaxSpeed(10.f) | 구현 확인 |
| V13 | 버프 NetUpdateFrequency=1 | ACXPickup 공통 설정 | 구현 확인 |
| V14 | 획득 버프 파괴 | ACXPickup 공통 파괴 처리 | 구현 확인 |
| V15 | 버프 파괴 시 파티클·사운드 | Destroyed → PlayCollectionEffects | 이번 검수에서 시점 수정 |
| V16 | 스포너 클래스·스폰 | ACXItemSpawner, 서버 SpawnActor | 구현 확인 |
| V17 | 생성 객체 속성·파괴 구독 | SpawnedItem, OnDestroyed.AddDynamic | 구현 확인 |
| V18 | 파괴 후 2초 같은 종류 리스폰 | RespawnDelay=2, ItemClass로 재생성 | 구현 확인 |
| V19 | 레벨 스포너 배치·종류 지정 | 스포츠카 맵 코인9/버프3, 오프로드 맵 코인3/버프1 | 애셋 검사 확인 |

## GameMode — 13개

| 번호 | 요구사항 | 구현 근거 | 판정 |
|---|---|---|---|
| G1 | GameState 클래스 | ACXGameState | 구현 확인 |
| G2 | Waiting/Playing/Finished Enum | ECXMatchPhase | 구현 확인 |
| G3 | 현재 상태·남은 시간 | MatchPhase, RemainingTime 복제 | 구현 확인 |
| G4 | 우승자 목록 | WinnerPlayerIds, WinnerNames 복제 | 구현 확인 |
| G5 | 시작 시 Waiting | InitGameState | 구현 확인 |
| G6 | 첫 접속 시 대기 시작 | PostLogin에서 중복 타이머 없이 시작 | 구현 확인 |
| G7 | 대기 1초 감소·공통 UI | 1초 반복 서버 타이머, 복제 RemainingTime 표시 | 구현 확인 |
| G8 | 대기 끝 Playing | AdvanceMatch | 구현 확인 |
| G9 | 진행 1초 감소·공통 UI | 동일 서버 타이머와 HUD | 구현 확인 |
| G10 | 진행 끝 Finished | FinishMatch | 구현 확인 |
| G11 | 최고 원화 플레이어 탐색 | 서버 PlayerArray의 Coins 비교 | 구현 확인 |
| G12 | 동점 공동 우승 | 최고 점수와 같은 모든 플레이어 목록에 추가 | 구현 확인 |
| G13 | 자신의 승리·패배 UI | 로컬 PlayerId를 WinnerPlayerIds에서 검사 | 구현 확인 |

## 검수 범위와 동작 세부사항

- 핵심 로직은 C++이며 기존 위젯/차량 블루프린트와 입력 애셋을 사용합니다.
- 획득 보상과 충돌 차단은 즉시 적용합니다. 복제 전달을 위해 액터 수명은 0.25초 뒤 종료합니다.
  리스폰 2초는 액터의 실제 파괴 시점부터 셉니다.
- 5원 미만 보호는 코인 이전을 막는다는 뜻입니다. 공격 디버그 선과 피격 파티클은 충돌 시 표시합니다.
- 공격 시작 위치는 별도 범퍼 소켓이 아닌 메시 바운드 전면 중앙으로 계산합니다.
- 공격/아이템 획득은 Playing에서만 허용합니다.
- 자동 검증은 NullRHI로 실행하므로 실제 화면 배치, 파티클 렌더링, 사운드 청음은 검증 범위 밖입니다.
  이펙트/사운드 애셋 연결과 파괴 시 호출 여부는 자동 테스트로 검사합니다.
- 결과 로그와 재실행 방법은 Gameplay.md를 참고하세요.

## 이번 수정 후 검증 결과

- CXEditor Win64 Development 빌드 성공.
- CX.Gameplay.ServerRules 통과. 코인·버프의 파괴 전 미재생/파괴 시 재생 및 이펙트·사운드 애셋 할당 검사 포함.
- 서버 1개와 클라이언트 2개의 실제 네트워크 테스트 모두 PASS.
- 저장된 애셋 구성은 직전 AssetVerification의 ALL PASS 결과와 이번 코드 대조로 확인했으며, 이번 검수에서는 애셋을 변경하지 않았습니다.
- 판정: 요청 47개 항목의 구현·연결 확인 완료. 화면·사운드의 수동 시청각 검증은 미실시.
