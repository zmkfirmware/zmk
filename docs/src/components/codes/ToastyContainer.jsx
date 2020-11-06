/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import React from "react";
import { ToastContainer } from "react-toastify";
import "react-toastify/dist/ReactToastify.css";

export default function ToastyContainer() {
  return (
    <ToastContainer
      position="bottom-right"
      autoClose={2000}
      hideProgressBar={true}
      newestOnTop={true}
    />
  );
}

ToastyContainer.propTypes = {};
