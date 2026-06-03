import Heading from "@theme/Heading";

import { HardwareMetadata } from "../hardware-metadata";
import { groupedMetadata, InterconnectDetails } from "./hardware-utils";

interface HardwareListProps {
  items: HardwareMetadata[];
}

type ElementOrString = JSX.Element | string;

function itemHasMultiple(item: HardwareMetadata) {
  return (
    (item.type == "board" || item.type == "shield") &&
    (item.siblings?.length ?? 1) > 1
  );
}

function itemIds(item: HardwareMetadata) {
  if (item.type == "board" || item.type == "shield") {
    const nodes = (item.siblings ?? [item.id])
      .map((id) => <code key={id}>{id}</code>)
      .reduce<
        ElementOrString[]
      >((prev, curr, index) => [...prev, index > 0 ? ", " : "", curr], []);
    return <span key={item.id}>{nodes}</span>;
  } else {
    return <code key={item.id}>{item.id}</code>;
  }
}

const TYPE_LABELS: Record<
  HardwareMetadata["type"],
  Record<"singular" | "plural", string>
> = {
  board: { plural: "Boards: ", singular: "Board: " },
  shield: { singular: "Shield: ", plural: "Shields: " },
  interconnect: { singular: "Interconnect: ", plural: "Interconnects: " },
};

function HardwareLineItem({ item }: { item: HardwareMetadata }) {
  return (
    <li>
      <a href={item.url}>{item.name}</a> (
      {TYPE_LABELS[item.type][itemHasMultiple(item) ? "plural" : "singular"]}{" "}
      {itemIds(item)})
    </li>
  );
}

function mapInterconnect({
  interconnect,
  boards,
  shields,
}: InterconnectDetails) {
  if (!interconnect) {
    return null;
  }

  return (
    <div key={interconnect.id}>
      <Heading as="h3" id={interconnect.id}>
        {interconnect.name} Interconnect
      </Heading>
      {interconnect.description && <p>{interconnect.description}</p>}
      <Heading as="h4">Boards</Heading>
      <ul>
        {boards.map((s) => (
          <HardwareLineItem key={s.id} item={s} />
        ))}
      </ul>
      <Heading as="h4">Shields</Heading>
      <ul>
        {shields.map((s) => (
          <HardwareLineItem key={s.id} item={s} />
        ))}
      </ul>
    </div>
  );
}

function HardwareList({ items }: HardwareListProps) {
  const grouped = groupedMetadata(items);

  return (
    <>
      <section>
        <Heading as="h2" id="composite">
          Composite Keyboards
        </Heading>
        <p>
          Composite keyboards are composed of two main PCBs: a small controller{" "}
          <strong>board</strong> with exposed pads, and a larger keyboard PCB (a{" "}
          <strong>shield</strong>, in ZMK lingo) with switch footprints. The
          board and shield share the same <strong>interconnect</strong>{" "}
          standard, which defines the physical and electrical specifications for
          the PCB-to-PCB connection.
        </p>
        <p>
          Boards and shields that share the same interconnect are usually
          compatible with each other but not always. Check hardware
          compatibility before connecting them.
        </p>
        <p>
          Designing a custom composite keyboard with an off-the-shelf controller
          board? Check out the{" "}
          <a href="/docs/hardware-integration/new-shield">
            New Keyboard Shield
          </a>{" "}
          guide.
        </p>

        {Object.values(grouped.interconnects).map(mapInterconnect)}
      </section>
      <section>
        <Heading as="h2" id="onboard">
          Onboard Controller Keyboards
        </Heading>
        <p>
          Keyboards with onboard controllers are single PCBs that contain all
          the components of a keyboard, including the controller chip, switch
          footprints, etc.
        </p>
        <p>
          Designing a custom keyboard with an onboard controller? Check out the{" "}
          <a href="/docs/hardware-integration/new-board">New Board</a> guide.
        </p>
        <ul>
          {grouped["onboard"]
            .sort((a, b) => a.name.localeCompare(b.name))
            .map((s) => (
              <HardwareLineItem key={s.id} item={s} />
            ))}
        </ul>
      </section>
    </>
  );
}

export default HardwareList;
