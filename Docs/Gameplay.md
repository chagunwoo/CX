# 차량 멀티플레이 기능

## 실행

`VehicleAdvExampleMap` 또는 `VehicleOffroadExampleMap`에서 Play를 실행합니다.
멀티플레이 확인은 에디터 Play 설정의 Number of Players를 2 이상으로 설정합니다.

- 첫 플레이어 접속 후 **10초 대기 → 120초 진행 → 종료**.
- **Enter**: 채팅 입력. 다시 Enter로 전송하고 운전 입력으로 복귀.
- **마우스 좌클릭**: 앞 범퍼에서 15m 라인트레이스 공격. 서버 쿨다운 0.5초.
- 피해 차량이 5원 이상일 때만 정확히 5원을 이전합니다.
- 원반 모양 코인: 10원. 원뿔 모양 버프: 최대 속도 +10km/h.
- 기본 속도 제한은 120km/h이며, 실제 차량 물리 속도에도 보너스를 적용합니다.
- 아이템은 획득 처리 즉시 숨김/충돌 해제 후 0.25초 뒤 파괴되며, 파괴 시 파티클·사운드를 재생합니다.
  스포너는 파괴 이벤트로부터 2초 후 같은 아이템을 생성합니다.
- 공격과 아이템 획득은 Playing에서만 가능합니다. 채팅은 모든 단계에서 가능합니다.
- HUD는 자신의 코인/속도 보너스, 공통 경기 상태/시간, 승리·패배·공동 우승을 표시합니다.

## 구성

- `ACXPlayerState`: `Coins`, `BonusMaxSpeed`를 `COND_OwnerOnly`로 복제. 값 변경은 서버만 허용.
- `ACXGameState`: 단계, 남은 시간, 우승자 ID/이름 복제. 다른 플레이어의 코인 잔고는 공개하지 않습니다.
- `ACXGameMode`: 최초 접속 타이머 시작, 최고 잔고 비교, 동점 공동 우승, 전체 퇴장 시 대기 상태 초기화.
- `ACXPawn`: InputAction 연결, 공격 Server RPC, 벽에 막히는 라인트레이스, ApplyDamage, 공격 이펙트.
- `ACXCoin`, `ACXSpeedBuff`: 서버 오버랩 보상, 중복 획득 차단, 1Hz 복제, 파티클/사운드.
- `ACXItemSpawner`: 서버에서만 생성, OnDestroyed 구독, 2초 리스폰.
- `UCXMatchHUD`: C++에서 생성하는 개인 능력치 및 경기 상태 UI.
- `UChatUI`, `UCXUI`: 기존 위젯 블루프린트 활용. 채팅 입력/전송/표시 로직은 C++.
- `ACXPlayerController`: 채팅 Server/Client RPC, UI 준비 전 메시지 보관, 로컬 위젯 생성.
- `UChatSubsystem`: 서버의 최근 채팅 10개 보관. 새 접속자에게 기록 전달.

## 애셋과 맵

기존 `/Game/UI/WBP_Chat`의 실제 위젯 이름에 맞춰 `MultiLineEditableTextBox_ChatBox` 바인딩을 수정했습니다.
기존 `/Game/VehicleTemplate/Blueprints/W_VehicleAdvUI`와 PlayerController 연결을 사용합니다.
`/Game/VehicleTemplate/Input/Actions/IA_Attack`을 생성하고 스포츠카/오프로드카 블루프린트에 지정했습니다.
좌클릭 매핑 컨텍스트는 C++에서 생성합니다.

- VehicleAdvExampleMap: 코인 스포너 9개, 버프 스포너 3개.
- VehicleOffroadExampleMap: 코인 스포너 3개, 버프 스포너 1개.
- 플레이어 시작 지점 앞의 지면에 배치하고, 월드 파티션 거리와 무관하게 스포너가 유지되도록 설정했습니다.
- 수정 전 애셋/맵 백업: `Saved/ImplementationBackup/`.
- `Scripts/setup_gameplay_assets.py`는 애셋 연결 및 배치를 재현하는 에디터 전용 스크립트입니다. 게임 실행에는 Python 플러그인이 필요하지 않습니다.

## 설정

게임모드 블루프린트의 WaitingDuration, PlayingDuration으로 시간을 변경할 수 있습니다.
차량의 AttackRange, AttackCooldown, BaseMaxSpeed, HitEffect를 조정할 수 있습니다.
스포너의 ItemClass, RespawnDelay로 아이템 종류와 리스폰 시간을 지정합니다.
아이템의 PickupEffect, PickupSound를 교체할 수 있습니다.

## 검증

`CX.Gameplay.ServerRules` 자동화 테스트는 서버 권한, 잔고 경계값, OwnerOnly 복제 설정,
공격과 쿨다운, 아이템 중복 획득, 리스폰, 경기 단계/동점 결과, 채팅 기록을 확인합니다.

개발 빌드의 `-CXNetworkSmoke`는 테스트 전용 옵션입니다. 별도 서버와 클라이언트 2개로
실제 OwnerOnly 복제, 위젯 바인딩, 채팅 RPC, 아이템 개수, 공동 우승 결과를 확인하고 종료합니다.
**일반 플레이에는 이 옵션을 사용하지 마세요.** 테스트는 임시 코인/보너스/경기 시간을 설정합니다.

### 최종 확인 결과 (2026-10-27)

- CXEditor / Win64 / Development 빌드 성공.
- `CX.Gameplay.ServerRules` 성공: 권한 검사, 잔고 경계값, 실제 범퍼 트레이스,
  쿨다운, 실제 메시 오버랩, 중복 보상 방지, 2초 리스폰, 단독/공동 우승,
  채팅 기록/전송 제한, 마지막 플레이어 퇴장 초기화 확인.
- 별도 서버 1개 + 클라이언트 2개 테스트 모두 PASS.
  각 클라이언트에서 자신만 코인 25와 보너스 10/20을 받고, 다른 플레이어의 비공개 값은 0으로 유지됨.
  양쪽 채팅 2개 표시, 위젯 바인딩, 아이템 12개(중복 없음), 공동 우승자 2명 및 남은 시간 0 확인.
- 애셋 검사 PASS: 두 맵의 코인/버프 개수, 스포너 복제 설정, 게임모드 클래스,
  공격 InputAction 연결, 위젯/차량 블루프린트 컴파일 확인.
- 테스트는 NullRHI(화면 렌더링 없는 실행)로 수행했습니다. 시각 디자인과 파티클·사운드의
  실제 출력에 대한 육안/청음 검사는 포함하지 않았습니다.
- 서버 규칙 테스트에는 임시 월드 정리 단계의 EndPlay 경고 1개가 있습니다. 검증 실패는 0개입니다.

재실행: `Scripts/RunGameplayChecks.ps1 -Mode All`.
결과: `Saved/Logs/ImplementationBuild.log`, `GameplayTests.log`, `NetworkServer.log`,
`NetworkClient1.log`, `NetworkClient2.log`, `AssetVerification.log`.
자동화 보고서: `Saved/Tests/Automation/index.json`.

