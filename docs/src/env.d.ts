// We're using `virtual:starlight/user-config` in component overrides,
// which is not exposed by default and is technically an internal API.
declare module "virtual:starlight/user-config" {
  const config: import("@astrojs/starlight/types").StarlightConfig;
  export default config;
}
