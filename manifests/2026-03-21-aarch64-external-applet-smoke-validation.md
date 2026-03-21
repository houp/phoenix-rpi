# Manifest: First External-Applet Shell Smoke Validation

- Date: `2026-03-21`
- Step: `STEP-0264`
- Scope: validate the first external-applet shell command on the fast QEMU lanes

## Command Under Test

- `ls /`

## Result

- the command echo is visible in the generic lane:
  - `ls /`
- but the selected success markers are not valid for this smoke shape:
  - `dev` already appears in earlier boot logs such as `create_dev: ...`
  - `syspage` can also appear before the command output band
- so this command does not provide a clean deterministic external-applet check
  with the current simple matcher

## Conclusion

- the next external-applet smoke should use a unique output token rather than
  a filesystem listing that collides with earlier boot text
- the next bounded step should therefore select a unique-token applet command,
  for example `echo <token>`
