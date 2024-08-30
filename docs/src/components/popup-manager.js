import { useEffect, useState } from "react";
import { useLocation } from "@docusaurus/router";
import Popup from "./popup";

const PopupManager = () => {
  const location = useLocation();
  const [showPopup, setShowPopup] = useState(false);

  const categoryPath = "/docs/development";
  const redirectPath = "/docs/development/contributing/clean-room";

  useEffect(() => {
    // Check if the current path is within the desired category
    const isCategoryPage = location.pathname.startsWith(categoryPath);

    if (isCategoryPage) {
      // Check localStorage to see if the popup has been shown before
      const hasSeenPopup = localStorage.getItem("hasSeenCleanRoomWarningPopup");
      // If clean room is clicked directly no redirect necessary
      if (window.location.pathname != redirectPath && !hasSeenPopup) {
        setShowPopup(true);
        localStorage.setItem("hasSeenCleanRoomWarningPopup", "true");
      }
    }
  }, [location.pathname]);

  const handleClose = () => {
    window.location.href = redirectPath;
    setShowPopup(false);
  };

  return showPopup ? <Popup onClose={handleClose} /> : null;
};

export default PopupManager;
