import { makeScene2D } from "@motion-canvas/2d/lib/scenes";
import { makeRefs } from "@motion-canvas/core/lib/utils";
import { all, chain, delay, waitFor } from "@motion-canvas/core/lib/flow";
import Key, { KeyTravel } from "../../../components/Key";
import Output from "../../../components/Output";
import { linear } from "@motion-canvas/core/lib/tweening";

export default makeScene2D(function* (view) {
  const tap = makeRefs<typeof Key>();
  view.add(<Key refs={tap} binding={"&ht_tp"} params={"\u21e7 F"} />);
  tap.group.position.x(-400);
  tap.group.position.y(-150);
  tap.duration.fill("#D9D9D9");

  const tap_output = makeRefs<typeof Output>();
  view.add(<Output refs={tap_output} />);
  tap_output.group.position(tap.group.position());
  tap_output.group.position.y(tap_output.group.position.y() + 300);

  const hold = makeRefs<typeof Key>();
  view.add(<Key refs={hold} binding={"&ht_tp"} params={"\u21e7 F"} />);
  hold.group.position.x(400);
  hold.group.position.y(-150);
  hold.duration.fill("#D9D9D9");

  const hold_output = makeRefs<typeof Output>();
  view.add(<Output refs={hold_output} />);
  hold_output.group.position(hold.group.position());
  hold_output.group.position.y(hold_output.group.position.y() + 300);

  yield* waitFor(0.5);
  yield* all(
    tap.body.position.y(KeyTravel, 0.15),
    hold.body.position.y(KeyTravel, 0.15)
  );
  yield* all(
    tap.duration.grow(0.5, 1, linear),
    delay(1, tap.body.position.y(0, 0.15)),
    hold.duration.grow(1, 2, linear)
  );
  yield* chain(
    all(tap.group.rotation(3, 0.03), hold.group.rotation(3, 0.03)),
    all(tap.group.rotation(-3, 0.06), hold.group.rotation(-3, 0.06)),
    all(tap.group.rotation(0, 0.03), hold.group.rotation(0, 0.03)),
    all(tap_output.output.text("f", 0), hold_output.shift.fill("#969696", 0.15))
  );
  yield* waitFor(0.25);
  yield* hold.body.position.y(0, 0.15);
  yield* delay(
    0.5,
    all(tap.duration.grow(0, 0.15), hold.duration.grow(0, 0.15))
  );
});
