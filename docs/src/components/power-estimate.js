/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

import PropTypes from "prop-types";
import {
  displayPower,
  underglowPower,
  backlightPower,
  zmkBase,
} from "../data/power";
import "../css/power-estimate.css";

// Average monthly discharge percent
const lithiumIonMonthlyDischargePercent = 5;
// Average voltage of a lithium ion battery based of discharge graphs
const lithiumIonAverageVoltage = 3.8;
// Average discharge efficiency of li-ion https://en.wikipedia.org/wiki/Lithium-ion_battery
// via https://web.archive.org/web/20090326150713/http://www.pluginhighway.ca/PHEV2007/proceedings/PluginHwy_PHEV2007_PaperReviewed_Valoen.pdf
// assuming <= 1C discharge rate
const lithiumIonDischargeEfficiency = 1.0;
// Range of the discharge efficiency
const lithiumIonDischargeEfficiencyRange = 0.01;

// Proportion of time spent typing (keys being pressed down and scanning). Estimated to 2%.
const timeSpentTyping = 0.02;

// Nordic power profiler kit accuracy
const measurementAccuracy = 0.2;

const batVolt = lithiumIonAverageVoltage;

const palette = [
  "#bbdefb",
  "#90caf9",
  "#64b5f6",
  "#42a5f5",
  "#2196f3",
  "#1e88e5",
  "#1976d2",
];

function formatUsage(microWatts) {
  if (microWatts > 1000) {
    return (microWatts / 1000).toFixed(1) + "mW";
  }

  return Math.round(microWatts) + "µW";
}

function voltageEquivalentCalc(powerSupply) {
  if (powerSupply.type === "LDO") {
    return batVolt;
  } else if (powerSupply.type === "SWITCHING") {
    return powerSupply.outputVoltage / powerSupply.efficiency;
  }
}

function formatMinutes(minutes, precision, floor) {
  let message = "";
  let count = 0;

  let units = ["year", "month", "week", "day", "hour", "minute"];
  let multiples = [60 * 24 * 365, 60 * 24 * 30, 60 * 24 * 7, 60 * 24, 60, 1];

  for (let i = 0; i < units.length; i++) {
    if (minutes >= multiples[i]) {
      const timeCount = floor
        ? Math.floor(minutes / multiples[i])
        : Math.ceil(minutes / multiples[i]);
      minutes -= timeCount * multiples[i];
      count++;
      message +=
        timeCount + (timeCount > 1 ? ` ${units[i]}s ` : ` ${units[i]} `);
    }

    if (count == precision) return message;
  }

  return message || "0 minutes";
}

function PowerEstimate({
  board,
  splitType,
  batteryMilliAh,
  usage,
  underglow,
  backlight,
  display,
}) {
  if (!board || !board.powerSupply.type || !batteryMilliAh) {
    return (
      <div className="powerEstimate">
        <h3>
          <span>{splitType !== "standalone" ? splitType + ": " : " "}...</span>
        </h3>
        <div className="powerEstimateBar">
          <div
            className="powerEstimateBarSection"
            style={{
              width: "100%",
              background: "#e0e0e0",
              mixBlendMode: "overlay",
            }}
          ></div>
        </div>
      </div>
    );
  }

  const powerUsage = [];
  let totalUsage = 0;
  let decay = 0;

  const voltageEquivalent = voltageEquivalentCalc(board.powerSupply);

  // Lithium ion self discharge
  decay += lithiumIonMonthlyDischargePercent / 100;

  let batteryPowerUsageIndex = powerUsage.length;
  powerUsage.push({
    title: "Battery Self Discharge",
    usage: 0, // not yet known
  });

  // Quiescent current
  const quiescentMicroATotal =
    parseInt(board.powerSupply.quiescentMicroA) +
    parseInt(board.otherQuiescentMicroA);
  const quiescentMicroW = quiescentMicroATotal * voltageEquivalent;

  totalUsage += quiescentMicroW;
  powerUsage.push({
    title: "Board Quiescent Usage",
    usage: quiescentMicroW,
  });

  // ZMK overall usage
  const zmkMicroA =
    zmkBase[splitType].idle +
    (splitType !== "peripheral" ? zmkBase.hostConnection * usage.bondedQty : 0);

  const zmkMicroW = zmkMicroA * voltageEquivalent;
  const zmkUsage = zmkMicroW * (1 - usage.percentAsleep);

  totalUsage += zmkUsage;
  powerUsage.push({
    title: "ZMK Base Usage",
    usage: zmkUsage,
  });

  // ZMK typing usage
  const zmkTypingMicroA = zmkBase[splitType].typing * timeSpentTyping;

  const zmkTypingMicroW = zmkTypingMicroA * voltageEquivalent;
  const zmkTypingUsage = zmkTypingMicroW * (1 - usage.percentAsleep);

  totalUsage += zmkTypingUsage;
  powerUsage.push({
    title: "ZMK Typing Usage",
    usage: zmkTypingUsage,
  });

  if (underglow.glowEnabled) {
    const underglowAverageLedMicroA =
      underglow.glowBrightness *
        (underglowPower.ledOn - underglowPower.ledOff) +
      underglowPower.ledOff;

    const underglowMicroA =
      underglowPower.firmware +
      underglow.glowQuantity * underglowAverageLedMicroA;

    const underglowMicroW = underglowMicroA * voltageEquivalent;

    const underglowUsage = underglowMicroW * (1 - usage.percentAsleep);

    totalUsage += underglowUsage;
    powerUsage.push({
      title: "RGB Underglow",
      usage: underglowUsage,
    });
  }

  if (backlight.backlightEnabled) {
    let backlightMicroA =
      ((board.powerSupply.outputVoltage - backlight.backlightVoltage) /
        backlight.backlightResistance) *
      1000000 *
      backlight.backlightBrightness *
      backlight.backlightQuantity;

    if (
      backlight.backlightBrightness > 0 &&
      backlight.backlightBrightness < 1
    ) {
      backlightMicroA += backlightPower.pwmPower;
    }

    const backlightMicroW = backlightMicroA * voltageEquivalent;
    const backlightUsage = backlightMicroW * (1 - usage.percentAsleep);

    totalUsage += backlightUsage;
    powerUsage.push({
      title: "Backlight",
      usage: backlightUsage,
    });
  }

  if (display.displayEnabled && display.displayType) {
    const { activePercent, active, sleep } = displayPower[display.displayType];

    const displayMicroA = active * activePercent + sleep * (1 - activePercent);
    const displayMicroW = displayMicroA * voltageEquivalent;
    const displayUsage = displayMicroW * (1 - usage.percentAsleep);

    totalUsage += displayUsage;
    powerUsage.push({
      title: "Display",
      usage: displayUsage,
    });
  }
  const mAh = parseInt(batteryMilliAh);
  const initialMicroWh = mAh * 1000 * batVolt;
  const initialWh = initialMicroWh / 1e6;
  const initialWMonths = initialWh / 24 / 30;

  const drain = 1 - decay;
  const averagePowerW = -totalUsage / 1e6;
  // We define a basic model of battery depletion as follows. This is used to calculate the lifetime
  // on a single charge.
  //
  // f(t) = a^t
  // f(t) = (e^ln(a))^t
  // f(t) = e^ln(a)t
  // f'(t) = ln(a) * e^ln(a)t
  // f'(t) = ln(a) * f(t)

  // -- this gives the definition of exponential decay that we will use --
  // decay(factor, t, f) = ln(factor)f
  // f'(t) = decay(a, t, f(t))

  // -- we'll use a model of the battery discharge where it constantly self discharges while having
  // -- a constant power draw
  // g'(t) = decay(drain, t, g(t)) + power
  // g'(t) = ln(drain)g(t) + power
  // g'(t) - ln(drain)g(t) = power
  // e^(-ln(drain)t)g'(t) - ln(drain)e^(-ln(drain)t)g(t) = power * e^(-ln(drain)t)
  // d(e^(-ln(drain)t)g(t))/dt = power * e^(-ln(drain)t) // Integration by parts in reverse
  // d(drain^(-t)g(t))/dt = power * drain^(-t)
  // drain^(-t)g(t) = power * -1/ln(drain) * drain^(-t) + K // Integrate
  // g(t) = -power/ln(drain) + K / drain^(-t)
  // g(t) = -power/ln(drain) + K * drain^t

  // g_0 = -power/ln(drain) + K * drain^0
  // g_0 = -power/ln(drain) + K
  // g_0 + power/ln(drain) = K

  // g(t) = -power/ln(drain) + (g_0 + power/ln(drain)) * drain^t
  //
  // And finally, solve for the depletion time.
  //
  // g(t) = 0
  // 0 = -power/ln(drain) + (g_0 + power/ln(drain)) * drain^t
  // 0 = -used + (g_0 + used) * drain^t
  // used/(g_0 + used) = drain^t
  // ln(used/(g_0 + used))/ln(drain) = t
  const used = averagePowerW/Math.log(drain);
  const depletedAtMonth = Math.log(used/(initialWMonths + used)) / Math.log(drain);
  const runtimeHours = depletedAtMonth * 30 * 24;
  
  // Fill in the effective power consumption of the battery self discharge
  const batteryPercentageDrain = (1 - (lithiumIonMonthlyDischargePercent/100))**depletedAtMonth;
  const batteryMicroWhUsage = batteryPercentageDrain * initialMicroWh;
  const batteryAveragePower = batteryMicroWhUsage / runtimeHours;

  powerUsage[batteryPowerUsageIndex].usage = batteryAveragePower;
  totalUsage += batteryAveragePower;

  // Calculate the average minutes of use
  const estimatedAvgMinutes = runtimeHours * 60;

  // Calculate worst case for battery life
  const worstLithiumIonDischargeEfficiency =
    lithiumIonDischargeEfficiency - lithiumIonDischargeEfficiencyRange;

  const estimatedWorstEffectiveMicroWH =
    batteryMilliAh * batVolt * worstLithiumIonDischargeEfficiency * 1000;

  const highestTotalUsage = totalUsage * (1 + measurementAccuracy);

  const estimatedWorstMinutes = Math.round(
    (estimatedWorstEffectiveMicroWH / highestTotalUsage) * 60
  );

  // Calculate range (+-) of minutes using average - worst
  const estimatedRange = estimatedAvgMinutes - estimatedWorstMinutes;

  return (
    <div className="powerEstimate">
      <h3>
        <span>{splitType !== "standalone" ? splitType + ": " : " "}</span>
        {formatMinutes(estimatedAvgMinutes, 2, true)} (±
        {formatMinutes(estimatedRange, 1, false).trim()})
      </h3>
      <div className="powerEstimateBar">
        {powerUsage.map((p, i) => (
          <div
            key={p.title}
            className={
              "powerEstimateBarSection" + (i > 1 ? " rightSection" : "")
            }
            style={{
              width: (p.usage / totalUsage) * 100 + "%",
              background: palette[i],
            }}
          >
            <div className="powerEstimateTooltipWrap">
              <div className="powerEstimateTooltip">
                <div>
                  {p.title} - {Math.round((p.usage / totalUsage) * 100)}%
                </div>
                <div style={{ fontSize: ".875rem" }}>
                  ~{formatUsage(p.usage)} estimated avg. consumption
                </div>
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}

PowerEstimate.propTypes = {
  board: PropTypes.Object,
  splitType: PropTypes.string,
  batteryMilliAh: PropTypes.number,
  usage: PropTypes.Object,
  underglow: PropTypes.Object,
  backlight: PropTypes.Object,
  display: PropTypes.Object,
};

export default PowerEstimate;
