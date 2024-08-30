import Footer from "@theme-original/Footer";
import PopupManager from "../../components/popup-manager"; // Adjust path if necessary

export default function FooterWrapper(props) {
  return (
    <>
      <PopupManager />
      <Footer {...props} />
    </>
  );
}
