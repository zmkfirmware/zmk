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
    let nodes = (item.siblings ?? [item.id])
      .map<ElementOrString>((id) => <code key={id}>{id}</code>)
      .reduce(
        (prev, curr, index) => [...prev, index > 0 ? ", " : "", curr],
        [] as ElementOrString[]
      );
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
  let grouped = groupedMetadata(items);

  return (
    <>
      <section>
        <Heading as="h2" id="onboard">
          Onboard Controller Keyboards
        </Heading>
        <p>
          Keyboards with onboard controllers are single PCBs that contain all
          the components of a keyboard, including the controller chip, switch
          footprints, etc.
        </p>
        <ul>
          {grouped["onboard"]
            .sort((a, b) => a.name.localeCompare(b.name))
            .map((s) => (
              <HardwareLineItem key={s.id} item={s} />
            ))}
        </ul>
      </section>
      <section>
        <Heading as="h2" id="composite">
          Composite Keyboards
        </Heading>
        <p>
          Composite keyboards are composed of two main PCBs: a small controller
          board with exposed pads, and a larger keyboard PCB (a shield, in ZMK
          lingo) with switch footprints and a location where the controller is
          added. This location is called an interconnect. Multiple interconnects
          can be found below.
        </p>
        {Object.values(grouped.interconnects).map(mapInterconnect)}
      </section>
    </>
  );
}

export default HardwareList;
