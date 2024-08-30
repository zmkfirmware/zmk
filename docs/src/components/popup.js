import "../css/popup.css";
import PropTypes from "prop-types";

const Popup = ({ onClose }) => {
  return (
    <div className="popup-overlay">
      <div
        className="popup-content theme-admonition theme-admonition-danger admonition_node_modules-@docusaurus-theme-classic-lib-theme-Admonition-Layout-styles-module alert alert--danger"
        role="alert"
      >
        <h1 className="centered-container"> Clean Room Policy </h1>
        <div className="centered-container">
          <p>
            Before reading this section, it is <b>vital</b> that you read
            through our clean room policy.
          </p>
        </div>
        <div className="centered-container">
          <button
            className="button button--outline button--secondary"
            onClick={onClose}
          >
            Take me there!
          </button>
        </div>
      </div>
    </div>
  );
};

export default Popup;

Popup.propTypes = {
  onClose: PropTypes.func.isRequired,
};
