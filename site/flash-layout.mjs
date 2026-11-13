// The merged factory image must never be used by the settings-preserving updater.
export function validUpdateParts(parts) {
  return validParts(parts, 'firmware/lighthouse-bridge-');
}
export function validMatterParts(parts) {
  return validParts(parts, 'firmware/matter/lighthouse-matter-');
}
export function validRsMatterParts(parts) {
  return validParts(parts, 'firmware/rs-matter/lighthouse-rs-matter-');
}
function validParts(parts, prefix) {
  const offsets = [0x1000, 0x8000, 0xe000, 0x10000];
  const names = ['bootloader', 'partitions', 'boot-app0', 'app'];
  return Array.isArray(parts) && parts.length === 4 && parts.every((part, i) =>
    part.offset === offsets[i] && typeof part.path === 'string' &&
    part.path.split('?').length === 2 &&
    part.path.split('?')[0] === `${prefix}${names[i]}.bin` &&
    /^sha=[a-f0-9]{64}$/.test(part.path.split('?')[1] || '')) &&
    new Set(parts.map(part => part.path)).size === 4;
}
