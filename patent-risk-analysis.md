# dali-ui 오픈소스 공개 시 특허 위험 분석

> **분석 일자**: 2026-05-08  
> **분석 범위**: dali-ui-foundation, dali-ui-components 전체 소스  
> **주안점**: UI/User Interaction 영역의 특허 침해 가능성

---

## 위험도 분류 기준

| 등급 | 의미 |
|------|------|
| 🔴 매우 높음 | 특허 소송 전례가 있거나 청구항 직접 충돌 가능성 높음 |
| 🟠 높음 | 알려진 특허 포트폴리오와 구현 방식이 유사함 |
| 🟡 중간 | 잠재적 충돌 가능성 있으나 구현 세부 사항에 따라 달라짐 |
| 🟢 낮음 | 업계 표준 기법 또는 특허 분쟁 사례 적음 |

---

## 1. Gesture / Touch Interaction

### 🔴 관성 스크롤 (Fling / Inertia Scrolling)

- **구현 파일**: `dali-ui-foundation/integration-api/scroll-view-impl.cpp`
- **구현 내용**: 손가락을 떼는 순간의 pan velocity를 `log(decelerationRate)` 기반 물리 공식으로 변환하여 최대 fling 거리를 계산하고 `EASE_OUT` 곡선으로 애니메이션
  - `mDecelerationRate = 0.998f`, `mMaxFlingDistance = 6000.0f`
- **특허 보유 의심 주체**: **Apple** — US7469381 등 iPhone 관성 스크롤 원천 특허군. Apple v. Samsung 소송의 핵심 쟁점이었던 영역

---

### 🟠 터치 인터셉트 시스템 (Touch Interception / Stealing)

- **구현 파일**: `dali-ui-foundation/integration-api/scroll-view-impl.cpp`
- **구현 내용**: 부모 View가 자식 View로 전달 중인 터치를 pan threshold 초과 시 가로채는(`OnInterceptTouch`) 메커니즘
- **특허 보유 의심 주체**: **Google** — Android `onInterceptTouchEvent()`와 구조적으로 동일
- **⚠️ 주의**: wiki 문서(`Touch-&-Gesture.md`)에 **"Same as Android"** 라고 직접 명시되어 있어 공개 시 증거로 활용될 위험 있음 → **문서 수정 권고**

---

### 🟡 LongPress → Pan 제스처 전환 (Gesture Handoff)

- **구현 파일**: `dali-ui-foundation/integration-api/interactive-trait-impl.cpp`
- **구현 내용**: Long-press 중 일정 이동 거리 초과 시 Pan 제스처로 전환하는 handoff 메커니즘
- **특허 보유 의심 주체**: **Apple** — US9547396, US8952895 등 gesture disambiguation/transition 특허군

---

## 2. Drag & Drop

### 🟠 Shadow View를 이용한 Drag 시각 피드백

- **구현 파일**: `dali-ui-foundation/internal/drag-drop-detector/drag-and-drop-detector-impl.cpp`
- **구현 내용**: PanGesture 드래그 시작 시 대상 View의 shadow 복사본(`mShadowView`)을 생성하여 화면에 오버레이, MOTION 이벤트로 위치 업데이트, UP 이벤트 시 드롭 대상에 신호 emit. Enter/Exit/Moved/Dropped 이벤트 시스템 포함
- **특허 보유 의심 주체**: **Apple** (US10175884 등), **Microsoft** (drag ghost/shadow 이미지 특허)

---

## 3. Focus Management

### 🟠 FocusFinder 방향 탐색 알고리즘 (AOSP 이식)

- **구현 파일**: `dali-ui-foundation/internal/focus-manager/focus-finder.cpp`
- **구현 내용**: UP/DOWN/LEFT/RIGHT 방향 포커스 이동 시 최적 후보를 계산하는 알고리즘
  - `MajorAxisDistance`, `MinorAxisDistance`, `GetWeightedDistanceFor`, `BeamBeats`, `IsBetterCandidate` 등
- **특허 보유 의심 주체**: **Google**
- **⚠️ 주의**: 파일 헤더에 **`Copyright (C) 2017 The Android Open Source Project, Modified by joogab yun`** 이 명시되어 있음. Apache 2.0 저작권 고지 의무는 충족되나, **특허권은 저작권과 별개**이므로 별도 법무 검토 필요

---

### 🟡 Reading Order 기반 Tab / Shift+Tab 포커스 순회

- **구현 파일**: `dali-ui-foundation/internal/focus-manager/focus-finder.cpp`
- **구현 내용**: View 계층을 y축 → RTL/LTR 기준 x축으로 stable_sort하여 reading order 결정 후 Tab(FORWARD) / Shift+Tab(BACKWARD) 이동
- **특허 보유 의심 주체**: **Microsoft**, **Apple**

---

### 🟡 포커스 히스토리 스택 & FocusGroup 경계

- **구현 파일**: `dali-ui-foundation/internal/focus-manager/focus-manager-impl.cpp`
- **구현 내용**: 포커스 이동 이력을 최대 30개 스택으로 관리, FocusGroup으로 포커스 경계 설정 (`MoveFocusBackward`)
- **특허 보유 의심 주체**: **Microsoft** (dialog/modal focus trap 관련 특허)

---

## 4. Scroll 동작

### 🟠 Pan Threshold 기반 스크롤 터치 탈취

- **구현 파일**: `dali-ui-foundation/integration-api/scroll-view-impl.cpp`
- **구현 내용**: `mPanThreshold = 5.0f` — 5px 이상 이동 시 스크롤 시작으로 판단하고 자식 View의 터치 시퀀스를 탈취하여 스크롤 처리
- **특허 보유 의심 주체**: **Apple**, **Google**

---

### 🟡 Over-Scroll (Rubber-Band) 모드

- **구현 파일**: `dali-ui-foundation/public-api/scroll-view.h`, `scroll-view-impl.cpp`
- **구현 내용**: `OverScrollMode` enum — Never / Always / ContentScrolls 세 가지 모드. 시각적 rubber-band 효과 API는 노출되어 있으나 현재 구현은 미완성
- **특허 보유 의심 주체**: **Apple** — US8539380 bounce-back 효과 특허
- **참고**: 시각 효과가 실제로 미구현인 경우 위험도 낮아질 수 있음

---

## 5. Text Input 처리

### 🟠 터치 기반 텍스트 선택 핸들 드래그

- **구현 파일**: `dali-ui-foundation/internal/text/decorator/text-decorator.cpp`, `internal/text/text-selection-handle-controller.cpp`
- **구현 내용**: 텍스트 선택 시 시작/끝 핸들을 Pan 제스처로 드래그하여 선택 영역 조정. 커서 500ms 주기 깜빡임, grab handle, 선택 highlight quad 렌더링. LongPress로 선택 시작
- **특허 보유 의심 주체**: **Apple** — US8552999 등 터치 기반 텍스트 선택 핸들 관련 다수 특허

---

### 🟡 비밀번호 마지막 문자 표시 (Show Last Character)

- **구현 파일**: `dali-ui-foundation/internal/text/hidden-text.cpp`
- **구현 내용**: 마지막 입력 문자를 `mPasswordRevealDuration` 동안 표시 후 마스킹 문자로 대체. `Mode::SHOW_LAST_CHARACTER` 방식
- **특허 보유 의심 주체**: **Apple**

---

### 🟡 Text Fit (자동 폰트 크기 조정)

- **구현 파일**: `dali-ui-foundation/integration-api/label-impl.cpp`, `internal/text/async-text/async-text-loader-impl.cpp`
- **구현 내용**: min/max 폰트 크기 범위 내에서 텍스트가 컨테이너에 맞도록 binary search로 최적 크기 탐색. `FitCandidate` 목록 기반 후보 탐색 방식도 지원 (`RenderTextFit`)
- **특허 보유 의심 주체**: **Apple** (`adjustsFontSizeToFitWidth` 관련), **Google**

---

### 🟡 Async Text Rendering (비동기 텍스트 렌더링)

- **구현 파일**: `dali-ui-foundation/internal/text/async-text/` 디렉토리 전체
- **구현 내용**: 텍스트 레이아웃·렌더링을 별도 스레드에서 비동기 처리. `AsyncTextManager`, `AsyncTextLoader`, `TextLoadingTask` 구성
- **특허 보유 의심 주체**: **Apple**, **Google**

---

## 6. 레이아웃 알고리즘

### 🟡 Text Exclusion Layout (배제 영역 텍스트 흐름)

- **구현 파일**: `dali-ui-foundation/internal/text/text-visualizer/exclusion-layout-cache.cpp`, `layout-engine.cpp`
- **구현 내용**: 지정 사각형 영역(`exclusionRegions`)을 피해 텍스트 줄이 배치되는 CSS Exclusion / Shapes 유사 기능. `BlockedInterval` 구조체로 줄별 배치 불가 구간 계산, y축 정렬 기반 효율적 캐시 사용
- **특허 보유 의심 주체**: **Apple** (DTP 스타일 텍스트 흐름), **Adobe** (InDesign 텍스트 레이아웃 특허)

---

### 🟢 Flex Layout 엔진

- **구현 파일**: `dali-ui-foundation/integration-api/layouts/flex-layout-impl.cpp`, `flex-layout-manager.cpp`
- **구현 내용**: CSS Flexbox 기반 레이아웃 (FlexDirection, FlexWrap, FlexJustify, FlexAlign)
- **위험도 낮은 이유**: CSS Flexbox는 W3C 표준이며 다수 오픈소스 구현 존재

---

## 7. 렌더링 기법

### 🟠 Background Blur Effect (Frosted Glass)

- **구현 파일**: `dali-ui-foundation/internal/render-effects/background-blur-effect-impl.cpp`, `gaussian-blur-effect-impl.cpp`, `gaussian-blur-algorithm.cpp`
- **구현 내용**: Downscale(기본 0.25x) + Gaussian blur radius(기본 40) 두 단계 적용. offscreen texture 렌더링 후 블러. `uAnimationRatio` uniform으로 블러 강도 애니메이션 가능
- **특허 보유 의심 주체**: **Apple** — iOS 7 이후 frosted glass / vibrancy 효과 관련 특허

---

### 🟢 Dissolve / Motion Blur / Motion Stretch 셰이더 효과

- **구현 파일**: `dali-ui-foundation/devel-api/shader-effects/dissolve-effect.cpp`, `motion-blur-effect.h`, 관련 GLSL 셰이더
- **구현 내용**: GPU 셰이더 기반 화면 전환 효과들
- **위험도 낮은 이유**: 셰이더 기반 전환 효과는 업계에서 광범위하게 사용되는 표준 기법

---

### 🟢 Mask Effect, Arc Visual, Conic Gradient, N-Patch Visual

- **구현 파일**: `internal/render-effects/mask-effect-impl.cpp`, `internal/visuals/arc/arc-visual.cpp`, `internal/visuals/gradient/conic-gradient.cpp`, `internal/visuals/npatch/npatch-visual.cpp`
- **위험도 낮은 이유**: 표준 GPU 렌더링 기법 또는 오랫동안 업계 관행화된 기법

---

## 8. Accessibility

### 🟢 AT-SPI 연동 접근성 시스템

- **구현 파일**: `dali-ui-foundation/integration-api/view-accessible.cpp`
- **구현 내용**: AT-SPI 표준 기반 접근성 구현. `AccessibilityRole` → AT-SPI Role 변환, 9-patch 포커스 하이라이트 오버레이
- **위험도 낮은 이유**: AT-SPI는 GNOME/Linux 오픈 표준

---

## 종합 위험도 표

| 위험도 | Feature | 주요 파일 | 특허 보유 의심 주체 |
|--------|---------|-----------|-------------------|
| 🔴 매우 높음 | 관성 스크롤 (Fling/Inertia) | `scroll-view-impl.cpp` | Apple |
| 🟠 높음 | 터치 인터셉트 시스템 | `scroll-view-impl.cpp` | Google |
| 🟠 높음 | FocusFinder 방향 탐색 (AOSP 이식) | `focus-finder.cpp` | Google |
| 🟠 높음 | 텍스트 선택 핸들 드래그 | `text-decorator.cpp` | Apple |
| 🟠 높음 | Background Blur Effect | `background-blur-effect-impl.cpp` | Apple |
| 🟠 높음 | Drag & Drop Shadow View | `drag-and-drop-detector-impl.cpp` | Apple, Microsoft |
| 🟠 높음 | Pan threshold 스크롤 터치 탈취 | `scroll-view-impl.cpp` | Apple, Google |
| 🟡 중간 | LongPress→Pan 제스처 전환 | `interactive-trait-impl.cpp` | Apple |
| 🟡 중간 | Over-Scroll rubber-band 모드 | `scroll-view.h` | Apple |
| 🟡 중간 | 비밀번호 마지막 문자 표시 | `hidden-text.cpp` | Apple |
| 🟡 중간 | Text Fit 자동 폰트 크기 조정 | `async-text-loader-impl.cpp` | Apple, Google |
| 🟡 중간 | Text Exclusion Layout | `exclusion-layout-cache.cpp` | Apple, Adobe |
| 🟡 중간 | Reading Order Tab 포커스 순회 | `focus-finder.cpp` | Microsoft, Apple |
| 🟡 중간 | 비동기 텍스트 렌더링 | `async-text/` | Apple, Google |
| 🟢 낮음 | Motion Blur / Dissolve 셰이더 | `shader-effects/` | — |
| 🟢 낮음 | AT-SPI 접근성 | `view-accessible.cpp` | — |
| 🟢 낮음 | Flex Layout, Arc Visual, N-Patch 등 | 각 impl | — |

---

## 즉시 조치 권고 사항

### 1. 법무팀 검토 필수 (오픈소스 공개 전)

| 우선순위 | 항목 | 이유 |
|---------|------|------|
| 1순위 | 관성 스크롤 구현 방식 | Apple과의 소송 전례가 있는 가장 민감한 영역 |
| 2순위 | FocusFinder 알고리즘 | AOSP 이식이 파일 내 명시됨 — 저작권과 별개로 특허 리스크 존재 |
| 3순위 | Background Blur | Apple의 frosted glass 특허 포트폴리오와 구현 방식 대조 필요 |
| 4순위 | 텍스트 선택 핸들 | Apple의 텍스트 편집 UI 특허는 지속적으로 분쟁이 많았던 영역 |

### 2. 문서 수정 (공개 전 필수)

- **wiki `Touch-&-Gesture.md`에서 "Same as Android" 표현 제거**  
  → 공개 문서에 이 표현이 남아 있으면 AOSP 복제의 직접적 증거로 활용될 수 있음

### 3. 파일별 저작권 고지 확인

- `focus-finder.cpp` — AOSP 저작권 고지 유지 필요 (Apache 2.0 의무 사항)
- 코드 출처가 서드파티인 파일 전체 목록 재확인

---

> ⚠️ **면책 고지**: 이 문서는 소스 코드의 기술적 유사성을 기반으로 한 예비 검토이며, 실제 특허 침해 여부의 법적 판단은 특허 전문 변호사의 검토가 반드시 필요합니다.
