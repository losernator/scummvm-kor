# ScummVM-kor libretro Core

[ScummVM](http://scummvm.org) is an interpreter for point-and-click adventure games that can be used as a libretro core.
>1.9.0(2016-10-17)을 기반으로 ScummVM kor.에 있는 한국어 출력 기능을 추가해서 수정/변환한 버전입니다. [누리돌님의 소스코드](https://github.com/nuridol/scummvm-kor)를 적용하였습니다.

## Compiling

To compile the core, run the following:

```
cd backends/platform/libretro/build
make
```

# 게임별 한국어 모드 지정 방법
| 게임명              | 언어  | 모드  | 플랫폼 |
| ----------------- |:----:|:-----:|:-----:|
| 매니악 맨션          |Korean| V1   | DOS |
| 풀 쓰로틀            |Korean| V1   | DOS |
| 샘 & 맥스 히트 더 로드 |Korean| V1   | DOS |
| 룸 VGA Ver.        |Korean| V1   | DOS |
| 텐타클의 최후의 날     |Korean| V2   | DOS |
| 더 디그             |Korean| V2   | DOS |
| 룸 EGA Ver.        |Korean| V2   | DOS |
| 인디아나 존스3        |Korean| V2   | DOS |
| 인디아나 존스4        |Korean| V2   | DOS |
| 원숭이 섬의 비밀1     |Korean| V2   | DOS |
| 원숭이 섬의 비밀2     |Korean| V2   | DOS |

*V1 대상 게임은 sub 폴더 밑에 ``<게임ID>.dat`` 파일이 필요합니다.*

# 링크들
## ScummVM
http://scummvm.org

## Xcode 프로젝트 작성 방법
http://forums.scummvm.org/viewtopic.php?t=13965&postdays=0&postorder=asc&start=60

## ScummVM Kor.(한글화)
http://wonst719.cafe24.com/zbxe/about

## ScummVM Kor Patch v1.2.1k11(iOS 버전 기능 추가)
http://www.nextcube.org/board/browse/1111/4188
