/// <reference types="astro/client" />

declare module "virtual:starlight/user-config" {
  const config: Record<string, any>;
  export default config;
}

declare module "virtual:starlight/components/*" {
  const component: any;
  export default component;
}

declare module "virtual:starlight/user-images" {
  const images: Record<string, any>;
  export const logos: Record<string, any>;
}

declare module "virtual:starlight/project-context" {
  const project: Record<string, any>;
  export default project;
}

declare module "virtual:starlight/pagefind-config" {
  export const pagefindUserConfig: Record<string, any>;
}
