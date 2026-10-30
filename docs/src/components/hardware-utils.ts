import type {
  Board,
  HardwareMetadata,
  Interconnect,
  Shield,
} from "../hardware-metadata";

export interface InterconnectDetails {
  interconnect?: Interconnect;
  boards: Board[];
  shields: Shield[];
}

export interface GroupedMetadata {
  onboard: Board[];
  interconnects: Record<string, InterconnectDetails>;
}

function groupedBoard(
  agg: GroupedMetadata,
  board: Board,
  unclassified: string[]
) {
  if (board.features?.includes("keys")) {
    agg.onboard.push(board);
  } else if (board.exposes) {
    board.exposes.forEach((element) => {
      const ic = agg.interconnects[element] ?? {
        boards: [],
        shields: [],
      };
      ic.boards.push(board);
      agg.interconnects[element] = ic;
    });
  } else {
    unclassified.push(board.id);
  }

  return agg;
}

function groupedShield(agg: GroupedMetadata, shield: Shield) {
  shield.requires.forEach((id) => {
    const ic = agg.interconnects[id] ?? { boards: [], shields: [] };
    ic.shields.push(shield);
    agg.interconnects[id] = ic;
  });
  shield.exposes?.forEach((id) => {
    const ic = agg.interconnects[id] ?? { boards: [], shields: [] };
    ic.shields.push(shield);
    agg.interconnects[id] = ic;
  });
  return agg;
}

function groupedInterconnect(agg: GroupedMetadata, item: Interconnect) {
  const ic = agg.interconnects[item.id] ?? { boards: [], shields: [] };
  ic.interconnect = item;
  agg.interconnects[item.id] = ic;

  return agg;
}

export function groupedMetadata(items: HardwareMetadata[]) {
  const unclassified: string[] = [];
  const grouped = items.reduce<GroupedMetadata>(
    (agg, hm) => {
      switch (hm.type) {
        case "board":
          return groupedBoard(agg, hm, unclassified);
        case "shield":
          return groupedShield(agg, hm);
        case "interconnect":
          return groupedInterconnect(agg, hm);
      }

      return agg;
    },
    { onboard: [] as Board[], interconnects: {} }
  );

  if (unclassified.length > 0) {
    console.warn(
      `Boards without keys or interconnect: ${unclassified.join(", ")}`
    );
  }

  return grouped;
}
