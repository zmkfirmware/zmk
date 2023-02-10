import { Rect, Text } from "@motion-canvas/2d/lib/components";
import { makeRef } from "@motion-canvas/core/lib/utils";
import { KeyBorderThickness } from "./Key";

const OutputThickness = KeyBorderThickness / 2;
const OutputFontSize = 40;
const OutputFontWeight = 600;
const ModifierRadius = 10;
const ModifierFontSize = 32;
const ModifierFontWeight = 600;
const ModifierMargin = 20;

export default function Output({
  refs,
}: {
  refs: {
    group: Rect;
    output: Text;
    shift: Rect;
    alt: Rect;
    ctrl: Rect;
    gui: Rect;
  };
}) {
  return (
    <Rect layout ref={makeRef(refs, "group")} direction={"column"}>
      <Rect
        layout
        direction={"row"}
        alignItems={"center"}
        justifyContent={"center"}
        radius={ModifierRadius}
        stroke={"#000000"}
        lineWidth={OutputThickness}
      >
        <Text
          ref={makeRef(refs, "output")}
          text={"&#8203"}
          fill={"#000000"}
          justifyContent={"center"}
          fontWeight={OutputFontWeight}
          fontSize={OutputFontSize}
          fontFamily={"sans-serif"}
          margin={ModifierMargin}
        />
      </Rect>
      <Rect layout direction={"row"} gap={20} marginTop={20}>
        <Rect
          layout
          ref={makeRef(refs, "shift")}
          radius={ModifierRadius}
          stroke={"#000000"}
          lineWidth={OutputThickness}
          grow={1}
          fill={"#D9D9D9"}
        >
          <Text
            fill={"#000000"}
            text={() => "SHIFT"}
            fontWeight={ModifierFontWeight}
            fontSize={ModifierFontSize}
            fontFamily={"sans-serif"}
            margin={ModifierMargin}
          />
        </Rect>

        <Rect
          layout
          ref={makeRef(refs, "alt")}
          radius={ModifierRadius}
          stroke={"#000000"}
          lineWidth={OutputThickness}
          grow={1}
          fill={"#D9D9D9"}
        >
          <Text
            fill={"#000000"}
            text={() => "ALT"}
            fontWeight={ModifierFontWeight}
            fontSize={ModifierFontSize}
            fontFamily={"sans-serif"}
            margin={ModifierMargin}
          />
        </Rect>

        <Rect
          layout
          ref={makeRef(refs, "ctrl")}
          radius={ModifierRadius}
          stroke={"#000000"}
          lineWidth={OutputThickness}
          grow={1}
          fill={"#D9D9D9"}
        >
          <Text
            fill={"#000000"}
            text={() => "CTRL"}
            fontWeight={ModifierFontWeight}
            fontSize={ModifierFontSize}
            fontFamily={"sans-serif"}
            margin={ModifierMargin}
          />
        </Rect>

        <Rect
          layout
          ref={makeRef(refs, "gui")}
          radius={ModifierRadius}
          stroke={"#000000"}
          lineWidth={OutputThickness}
          grow={1}
          fill={"#D9D9D9"}
        >
          <Text
            fill={"#000000"}
            text={() => "GUI"}
            fontWeight={ModifierFontWeight}
            fontSize={ModifierFontSize}
            fontFamily={"sans-serif"}
            margin={ModifierMargin}
          />
        </Rect>
      </Rect>
    </Rect>
  );
}
