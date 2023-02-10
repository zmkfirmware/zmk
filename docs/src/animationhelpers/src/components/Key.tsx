import { Rect, Text, Node } from "@motion-canvas/2d/lib/components";
import { SignalValue } from "@motion-canvas/core/lib/signals";
import { makeRef } from "@motion-canvas/core/lib/utils";

const KeySize = 200;
const KeyRadius = 10;
export const KeyBorderThickness = 10;
export const KeyTravel = 40;

export default function Key({
  refs,
  binding,
  params,
}: {
  refs: {
    group: Node;
    body: Node;
    fill: Rect;
    duration: Rect;
    shadow: Rect;
    binding: Text;
    params: Text;
  };
  binding: SignalValue<string>;
  params: SignalValue<string>;
}) {
  return (
    <Node ref={makeRef(refs, "group")} y={-KeyTravel}>
      <Rect
        ref={makeRef(refs, "shadow")}
        width={KeySize}
        height={KeySize}
        y={KeyTravel}
        radius={KeyRadius}
        fill={"#000000"}
        stroke={"#000000"}
        lineWidth={KeyBorderThickness}
      />
      <Node ref={makeRef(refs, "body")}>
        <Rect
          layout
          ref={makeRef(refs, "fill")}
          direction={"column-reverse"}
          width={KeySize}
          height={KeySize}
          fill={"#FFFFFF"}
        >
          <Rect ref={makeRef(refs, "duration")} grow={0} fill={"#FFFFFF"} />
        </Rect>
        <Rect
          width={KeySize}
          height={KeySize}
          fill={"#FFFFFF00"}
          radius={KeyRadius}
          stroke={"#000000"}
          lineWidth={KeyBorderThickness}
        />
        <Text
          ref={makeRef(refs, "binding")}
          text={binding}
          fill={"#000000"}
          width={KeySize}
          padding={15}
          fontWeight={600}
          fontSize={32}
          fontFamily={"sans-serif"}
          y={-65}
        />
        <Text
          ref={makeRef(refs, "params")}
          text={params}
          fill={"#000000"}
          width={KeySize}
          justifyContent={"center"}
          fontWeight={600}
          fontSize={60}
          fontFamily={"sans-serif"}
        />
      </Node>
    </Node>
  );
}
