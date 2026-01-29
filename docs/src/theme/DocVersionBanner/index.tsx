import { type ReactNode } from "react";
import clsx from "clsx";
import useDocusaurusContext from "@docusaurus/useDocusaurusContext";
import Link from "@docusaurus/Link";
import { ThemeClassNames } from "@docusaurus/theme-common";
import type { Props } from "@theme/DocVersionBanner";

function ZMKReleaseLink({ version }: { version: string }): ReactNode {
  return (
    <Link
      href={`https://v${version.replaceAll(".", "-")}-branch.zmk.dev/docs/`}
    >
      v{version}
    </Link>
  );
}

function DevWarningBanner({
  className,
  latestVersion,
}: Props & { latestVersion: string }): ReactNode {
  return (
    <div
      className={clsx(
        className,
        ThemeClassNames.docs.docVersionBanner,
        "alert alert--warning margin-bottom--md"
      )}
      role="alert"
    >
      You're viewing the documentation for the development version of ZMK. You
      may want the latest release <ZMKReleaseLink version={latestVersion} />.
    </div>
  );
}

export default function DocVersionBanner({ className }: Props): ReactNode {
  const {
    siteConfig: { customFields },
  } = useDocusaurusContext();

  if (
    !customFields?.releaseVersions ||
    !Array.isArray(customFields.releaseVersions)
  ) {
    return null;
  }

  const releaseVersions: [string] = customFields.releaseVersions as [string];

  if (customFields.isDevelopmentVersion) {
    return (
      <DevWarningBanner
        className={className}
        latestVersion={releaseVersions[0]}
      />
    );
  }
  return null;
}
