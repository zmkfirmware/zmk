### Revisions

It is common for hardware devices to come in multiple versions, while having the same name. The optional `revisions` array allows the tracking of multiple versions of the same device, which may have different pinouts. For example, the `mikoto` comes in the following versions: `5.20`, `7.1`, `7.3`. The corresponding `revisions` array is thus:

```yaml
revisions:
  - "5.20"
  - "7.1"
  - "7.3"
```

The corresponding files would then be named e.g. `mikoto_5_20.overlay`, `mikoto_7_1.overlay`, etc.

If the `revisions` property is used, then an additional property `default_revision` must exist and point to one of the revisions.
