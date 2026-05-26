# dali-ui 특허 리스크 후보 분석

## 검토 범위

- 대상: `dali-ui` 폴더 아래 전체 소스 코드
- 관점: 소스 코드 및 구현 기능/피처에서 특허 검토가 필요해 보이는 요소 식별
- 제외: 사용자 인터랙션 관련 구현 전반
  - touch/gesture/focus/drag/drop/selection/scroll event 전달 등
  - `internal/focus-manager/focus-finder.cpp`도 이번 정리에서 제외

> 본 문서는 법률 의견이 아니라 소스 코드 기반 기술 리스크 스크리닝입니다. 실제 특허 침해 여부 판단은 관련 특허 청구항과 제품 적용 범위를 기준으로 별도 검토가 필요합니다.

## 요약

현재 소스에서 특정 특허 침해가 명백해 보이는 코드는 확인되지 않았습니다. 다만 독자 구현이 있고, 제품 기능으로 노출될 경우 특허 검토 대상이 될 수 있는 후보는 아래 순서로 보입니다.

| 우선순위 | 영역 | 리스크 수준 | 검토 필요성 |
| --- | --- | --- | --- |
| 1 | TextVisualizer 비정형 영역 회피 텍스트 레이아웃 | 중간 | 제외 영역 기반 텍스트 흐름/배치 기능 |
| 2 | Glyph atlas/cache 및 geometry 재사용 렌더링 | 낮음~중간 | 텍스트 렌더링 성능 최적화 조합 |
| 3 | Background blur render effect | 낮음~중간 | 배경 캡처, downscale, two-pass blur, blur-once 조합 |
| 4 | Lottie/vector animation task 제어 | 낮음~중간 | marker range, loop/autoreverse, frame drop 보정, dynamic property |
| 5 | YUV/YUVA fast image texture upload | 낮음~중간 | plane texture, dummy chroma/alpha, unified shader 경로 |

## 특허 검토 후보

### 1. TextVisualizer 비정형 영역 회피 텍스트 레이아웃

관련 파일:

- `dali-ui-foundation/internal/text/text-visualizer/layout-engine.cpp`
- `dali-ui-foundation/internal/text/text-visualizer/exclusion-layout-cache.cpp`
- `dali-ui-foundation/internal/text/text-visualizer/prepared-text.*`

주요 구현:

- exclusion region을 top/bottom 기준으로 정렬하고 version을 관리
- 각 라인 높이와 겹치는 blocked interval을 산출
- blocked interval을 병합한 뒤 가용 interval을 계산
- glyph advance/width, line break 정보를 이용해 가용 interval 안에 glyph range를 배치
- placeholder cluster와 실제 glyph 배치를 분리 처리

리스크 판단:

- CSS Shapes, non-rectangular text flow, exclusion area 기반 text layout과 개념적으로 가까운 기능입니다.
- 개별 알고리즘은 일반적인 interval merge/line layout 방식이지만, UI 프레임워크 피처로 공개될 경우 특허 검토 대상으로 올릴 만합니다.
- 리스크 수준: **중간**

권장 확인:

- 이 기능이 외부 API 또는 제품 기능으로 노출되는지 확인
- “exclusion region around text”, “text flow around arbitrary shapes”, “non-rectangular text layout” 계열 특허 검색
- 기존 공개 표준/CSS Shapes와의 차이점 문서화

## 2. Glyph atlas/cache 및 geometry 재사용 렌더링

관련 파일:

- `dali-ui-foundation/internal/text/text-visualizer/rendering/text-visualizer-glyph-renderer.cpp`
- `dali-ui-foundation/internal/text/rendering/atlas/atlas-manager-impl.cpp`
- `dali-ui-foundation/internal/text/rendering/atlas/atlas-glyph-manager.*`
- `dali-ui-foundation/internal/text/rendering/atlas/atlas-mesh-factory.*`

주요 구현:

- glyph cache signature로 기존 glyph reference 재사용 여부 판단
- atlas별 mesh topology signature를 계산
- topology가 동일하면 geometry-only update로 전체 mesh rebuild 회피
- glyph bitmap atlas에 block 단위 배치, free list, reference count 관리
- atlas upload 시 2-pixel padding과 horizontal/vertical strip blit 적용

리스크 판단:

- glyph atlas, texture atlas, geometry update 최적화 자체는 널리 쓰이는 기술입니다.
- 다만 “glyph cache signature + atlas reference + topology signature + geometry-only update” 조합은 구현 특화성이 있어 검토 후보입니다.
- 리스크 수준: **낮음~중간**

권장 확인:

- 해당 TextVisualizer가 신규 차별화 기능인지, 기존 DALi text renderer와 동일 계열인지 확인
- 성능 최적화 설명 문서에서 특허성 있는 표현을 과도하게 사용하지 않도록 주의
- “glyph atlas cache invalidation”, “text mesh geometry reuse”, “texture atlas padding glyph rendering” 계열 검색

## 3. Background blur render effect

관련 파일:

- `dali-ui-foundation/internal/render-effects/background-blur-effect-impl.cpp`
- `dali-ui-foundation/internal/render-effects/gaussian-blur-algorithm.cpp`
- `dali-ui-foundation/public-api/render-effects/background-blur-effect.*`

주요 구현:

- owner view 뒤쪽 source를 별도 render task로 캡처
- downscale factor를 적용한 frame buffer 생성
- horizontal/vertical two-pass Gaussian blur 수행
- blur output을 target renderer texture로 주입
- `blurOnce`, blur strength animation, opacity animation 지원
- blur radius/downscale factor 제한 및 shader/uniform block cache 사용

리스크 판단:

- Gaussian blur와 separable two-pass blur는 일반적인 그래픽 기법입니다.
- 다만 UI background blur effect로서 render task 구성, once rendering, animation 제어를 조합한 제품 피처는 검토 대상입니다.
- 리스크 수준: **낮음~중간**

권장 확인:

- blur effect가 플랫폼 UI 핵심 차별화 기능인지 확인
- “real-time background blur UI”, “offscreen rendered background blur”, “downsampled blur UI effect” 계열 검색
- API 문서에서 특정 경쟁 제품 기능과 유사한 표현을 피함

## 4. Lottie/vector animation task 제어

관련 파일:

- `dali-ui-foundation/internal/visuals/animated-vector-image/vector-animation-task.cpp`
- `dali-ui-foundation/internal/visuals/animated-vector-image/vector-animation-manager.*`
- `dali-ui-foundation/internal/visuals/animated-vector-image/vector-animation-thread.*`

주요 구현:

- local/remote/encoded buffer 기반 vector animation load
- marker name 또는 frame number 기반 play range 지정
- loop count, autoreverse, stop behavior 지원
- frame rate와 speed factor 기반 frame duration 계산
- render 지연 시 dropped frame 계산
- dynamic property callback을 renderer에 전달
- layer/marker info lazy cache

리스크 판단:

- Lottie 렌더링 자체는 외부 renderer wrapper 성격이 강합니다.
- 다만 marker 기반 제어, dynamic property, frame drop 보정, worker thread rasterization scheduling이 제품 기능으로 묶이면 검토 후보입니다.
- 리스크 수준: **낮음~중간**

권장 확인:

- `VectorAnimationRenderer`의 실제 구현/라이선스/출처 확인
- Lottie 관련 third-party patent grant 또는 라이선스 조건 확인
- “vector animation playback range marker”, “dynamic property vector animation”, “frame drop vector animation rasterization” 계열 검색

## 5. YUV/YUVA fast image texture upload

관련 파일:

- `dali-ui-foundation/internal/image-loader/fast-track-loading-task.cpp`
- `dali-ui-foundation/internal/visuals/image/image-visual-shader-feature-builder.cpp`
- `dali-ui-foundation/internal/graphics/shaders/image-visual-shader.*`

주요 구현:

- fast track loading task에서 texture를 선생성
- YUV/YUVA plane loading 지원
- alpha plane이 없을 경우 dummy alpha pixel data 생성
- RGB image가 들어와도 unified shader 경로를 맞추기 위해 dummy U/V/A texture 사용
- shader feature builder에서 YUV/RGB/YUVA/unified shader variant 선택

리스크 판단:

- plane texture 및 shader 기반 색공간 변환은 일반적인 미디어 렌더링 방식입니다.
- “unified YUV and RGB shader path”와 dummy texture 처리 방식은 구현 특화성이 있어 검토 후보입니다.
- 리스크 수준: **낮음~중간**

권장 확인:

- 실제 사용 대상이 이미지인지 비디오/카메라 frame인지 확인
- “multi-plane image texture upload”, “YUV RGB unified shader”, “dummy chroma texture” 계열 검색
- 코덱/이미지 로더 라이브러리의 특허 라이선스 범위 확인

## 낮은 리스크로 본 영역

### Layout 시스템

관련 파일:

- `dali-ui-foundation/integration-api/layouts/flex-layout-manager.cpp`
- `dali-ui-foundation/integration-api/layouts/grid-layout-manager.cpp`
- `dali-ui-foundation/integration-api/layouts/stack-layout-manager.cpp`
- `dali-ui-foundation/integration-api/layouts/absolute-layout-manager.cpp`

판단:

- Flex/Grid/Stack/Absolute layout은 공개 표준 또는 일반 UI 프레임워크 구현에 가깝습니다.
- FlexLayout은 CSS Flexbox 모델을 따르는 것으로 보이며, 특허보다는 표준 호환성/저작권 구현 출처 검토가 더 적합합니다.
- 리스크 수준: **낮음**

### NPatch/Nine-patch

관련 파일:

- `dali-ui-foundation/internal/visuals/npatch/npatch-data.cpp`
- `dali-ui-foundation/internal/visuals/npatch/npatch-loader.cpp`
- `dali-ui-foundation/devel-api/utility/npatch-utilities.cpp`

판단:

- nine-patch/scale-9 계열은 오래된 공개 UI 기법에 가깝습니다.
- 특허보다는 asset format 호환성 및 기존 구현 출처 확인이 더 중요해 보입니다.
- 리스크 수준: **낮음**

### WebView

관련 파일:

- `dali-ui-foundation/integration-api/web-view-impl.cpp`
- `dali-ui-foundation/public-api/web-view.*`

판단:

- 자체 브라우저 엔진 구현이 아니라 `Dali::WebEngine` wrapper 및 texture/native image 연결에 가깝습니다.
- web engine 자체의 특허/라이선스 검토는 `dali-ui`보다 하위 dependency 범위에서 확인하는 것이 적절합니다.
- 리스크 수준: **낮음**

### Builder/Theme/Config/Shader generator

관련 파일:

- `dali-ui-foundation/internal/builder/*`
- `dali-ui-foundation/public-api/ui-config.*`
- `dali-ui-foundation/public-api/ui-theme-manager.*`
- `dali-ui-foundation/shader-generator/shader-generator.cpp`

판단:

- JSON parser/builder, theme/config manager, shader source generator는 일반적인 프레임워크 유틸리티입니다.
- 특허 리스크는 낮아 보입니다.
- 리스크 수준: **낮음**

## 제외 항목

아래 항목은 사용자 요청에 따라 이번 특허 리스크 정리에서 제외했습니다.

- touch/gesture 관련 event routing 및 recognition
- focus navigation/focus finder
- drag/drop detector
- text selection/cursor/clipboard/IME 등 user interaction 성격의 구현
- scroll event 전달 및 gesture 처리

특히 `dali-ui-foundation/internal/focus-manager/focus-finder.cpp`는 명시적으로 제외했습니다.

## 최종 의견

`dali-ui`에서 바로 문제로 볼 만한 명백한 특허 리스크는 확인되지 않았습니다. 다만 신규성/차별화 기능으로 보일 수 있는 구현은 TextVisualizer 쪽에 집중되어 있습니다.

검토 우선순위는 다음과 같습니다.

1. TextVisualizer 비정형 영역 회피 텍스트 레이아웃
2. TextVisualizer glyph atlas/cache 및 geometry 재사용 렌더링
3. Background blur render effect
4. Lottie/vector animation task 제어
5. YUV/YUVA fast image texture upload

실제 특허 검토 시에는 위 후보별로 제품 노출 여부, 공개 표준/선행기술 근거, 외부 dependency 라이선스 및 patent grant, 관련 특허 청구항 매핑을 함께 확인하는 것이 좋습니다.
