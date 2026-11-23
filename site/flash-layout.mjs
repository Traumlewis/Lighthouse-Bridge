export const boardFamilies = Object.freeze({
  esp32: 'ESP32',
  'weact-c3': 'ESP32-C3',
  'c3-supermini': 'ESP32-C3',
  's3-supermini': 'ESP32-S3',
  'xiao-c3': 'ESP32-C3',
  'xiao-s3': 'ESP32-S3',
});

export function validRsMatterParts(parts, board='esp32') {
  if (!Object.hasOwn(boardFamilies, board)) return false;
  const directory=board==='esp32'?'':board+'/';
  return validParts(parts, 'firmware/rs-matter/'+directory+'lighthouse-rs-matter-',board==='esp32'?0x1000:0);
}
function validParts(parts, prefix, bootOffset=0x1000) {
  const offsets = [bootOffset, 0x8000, 0xe000, 0x10000];
  const names = ['bootloader', 'partitions', 'boot-app0', 'app'];
  return Array.isArray(parts) && parts.length === 4 && parts.every((part, i) =>
    part.offset === offsets[i] && typeof part.path === 'string' &&
    part.path.split('?').length === 2 &&
    part.path.split('?')[0] === `${prefix}${names[i]}.bin` &&
    /^sha=[a-f0-9]{64}$/.test(part.path.split('?')[1] || '')) &&
    new Set(parts.map(part => part.path)).size === 4;
}
