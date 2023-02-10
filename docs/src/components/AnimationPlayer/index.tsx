import type { MotionCanvasPlayerProps } from "@motion-canvas/player";
import React, { ComponentProps } from "react";
import styles from "./styles.module.css";
import ExecutionEnvironment from "@docusaurus/ExecutionEnvironment";
import clsx from "clsx";

if (ExecutionEnvironment.canUseDOM) {
  import("@motion-canvas/player");
}

declare global {
  namespace JSX {
    interface IntrinsicElements {
      "motion-canvas-player": MotionCanvasPlayerProps & ComponentProps<"div">;
    }
  }
}

export interface AnimationPlayerProps {
  small?: boolean;
  auto?: boolean;
  name: string;
}

export default function AnimationPlayer({
  name,
  auto,
  small,
}: AnimationPlayerProps) {
  return (
    <div className={clsx(styles.container, small && styles.small)}>
      <motion-canvas-player
        class={styles.player}
        src={`/animations/${name}.js`}
        auto={auto}
      />
    </div>
  );
}
