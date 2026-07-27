# Accessibility Wiki Target Test

이 샘플은 [`wiki/Accessibility-(kr).md`](../../wiki/Accessibility-(kr).md)에
설명된 Accessibility API를 실제 타겟에서 학습하고 검증하기 위한 앱입니다.

기존 `accessibility-view-api`처럼 API 데이터 자체를 검사하는 것에 더해,
실제 Screen Reader가 사용하는 다음 왕복 경로를 함께 확인합니다.

```text
앱의 View metadata
  -> ViewAccessible
  -> DALi adaptor AT-SPI / D-Bus bridge
  -> 타겟 Screen Reader 발화와 highlight
  -> Screen Reader action/property 요청
  -> ViewImpl virtual callback
  -> 앱 화면과 [A11Y_WIKI_TARGET] 로그
```

## 코드 읽는 방법

구현 파일의 한글 주석은 wiki 순서와 동일하게 구성되어 있습니다.

- `[wiki: ...]`: 대응하는 wiki 섹션
- `[실타겟 테스트]`: Screen Reader에서 수행할 조작과 기대 결과
- 자체검사 주석: 앱 시작 시 타겟 프로세스에서 자동 실행되는 검사
- Custom View 주석: AT-SPI 요청이 `ViewImpl` virtual에 도달하는 경로

주요 구현은
[`accessibility-wiki-target-test-example.cpp`](accessibility-wiki-target-test-example.cpp)에
있습니다.

## API 커버리지

구분 | 포함된 API
--|--
기본 정보 | `Set/GetAccessibilityName`, `Description`, `Value`, `Role`
동작 속성 | `Set/IsAccessibilityHidden`, `Set/Reset/IsAccessibilityHighlightable`, `Set/IsAccessibilityScrollable`, `Set/IsAccessibilityModal`, `Set/GetAutomationId`
발화 정보 | `Add/Remove/Clear/HasAccessibilityReadingInfo`, `ReadingInfo` 4종
State | `Add/Remove/Clear/HasAccessibilityState`, `State` 5종
Relation | `Add/Remove/Clear/HasAccessibilityRelation`, 전체 유효 `RelationType`
번역 | `Set/Get/ClearTranslatableAccessibilityName/Description`, default/domain overload
언어 구간 | `Add/ClearAccessibilityNameLanguageSpan`, `Add/ClearAccessibilityDescriptionLanguageSpan`, 잘못된 범위 검사
Collection | `Set/IsAccessibilityCollectionContainer`, `Set/Get/ClearAccessibilityCollectionIndex`, `Set(-1)`
Highlight | `SetRequestInitialAccessibilityHighlight`, `IsInitialAccessibilityHighlightRequested`, `GrabAccessibilityHighlight`, `ClearAccessibilityHighlight`
Signal | `AccessibilityHighlightedSignal`, `AccessibilityReadingStatusChangedSignal`, `ReadingStatus` 5종
Custom View | activate, escape, value change, scroll-to-child, pan, zoom, 동적 name/description/value virtual
Raw attribute | `Append/Remove/ClearAccessibilityAttributes`, exported attribute map 확인

## 8개 테스트 페이지

1. 시작 시 실행된 전체 setter/getter 자체검사 결과
2. 기본 정보, 전체 Role 순환, hidden/highlightable/scrollable/modal
3. ReadingInfo와 ENABLED/SELECTED/CHECKED/BUSY/EXPANDED
4. relation, 번역 resource binding, code-point language span
5. collection container와 item index
6. 초기/실행 중 highlight, highlight signal, 읽기 lifecycle signal
7. `ViewImpl`의 9개 accessibility virtual
8. raw attribute와 `ClearAccessibilityAttributes()`의 typed metadata 제거 범위

각 페이지의 버튼은 Screen Reader double tap으로 실행합니다. 페이지 하단의
`다음 페이지` 버튼을 사용하면 8번 다음에 다시 1번으로 돌아옵니다.

## PASS 조건

앱 하단이 `RESULT PASS`가 되려면 다음 조건이 모두 충족되어야 합니다.

- 시작 시 API 자체검사가 전부 PASS
- AT-SPI bridge가 UP이고 Screen Reader가 ON
- 실제 highlight true/false signal 수신
- `GrabAccessibilityHighlight()`와 `ClearAccessibilityHighlight()` 성공
- 읽기 lifecycle 5개 status 수신
- 동적 name/description/value를 외부 client가 조회
- activate/value/escape/scroll/pan/zoom callback 실행
- accessibility-hidden sentinel이 외부 client에 노출되지 않음

pan/zoom/escape gesture를 제공하지 않는 Screen Reader 프로파일에서는 7번
페이지의 `escape pan zoom 보조 테스트` 버튼이 target 프로세스 안에서 해당
override를 실행합니다. activate와 value change는 실제 Screen Reader 조작으로
수행해야 합니다.

## Desktop build

```sh
cd samples/accessibility-wiki-target-test
cmake --fresh -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX .
make -j8
./bin/accessibility-wiki-target-test.example
```

Desktop에서는 레이아웃과 자체검사를 확인할 수 있지만, 외부 Screen Reader가
없으면 실타겟 결과는 `WAITING` 상태가 정상입니다.

## Tizen GBS build

저장소 root에서 실행합니다.

```sh
gbs build -A armv7l --include-all \
  --packaging-dir samples/accessibility-wiki-target-test/packaging
```

RPM을 타겟에 배포한 뒤 다음 app id로 실행합니다.

```sh
app_launcher -s com.samsung.dali.accessibility-wiki-target-test
```

## 타겟 로그

```sh
sdb dlog | grep A11Y_WIKI_TARGET
```

정상 완료 로그:

```text
[A11Y_WIKI_TARGET] RESULT=PASS wiki APIs and real target accessibility path verified
```

`SELF FAIL`, `GrabAccessibilityHighlight=false`, hidden sentinel 실패가 보이면
화면의 세부 결과와 해당 로그를 함께 확인합니다.
