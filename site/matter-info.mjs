// Experimental protocol: pairing codes stay in page memory and on physical USB.
export function parseMatterInfo(line) {
  if (!line.startsWith('LH_MATTER_INFO ')) return null;
  try {
    const value = JSON.parse(line.slice(15));
    if (value.protocol !== 1 || !/^\d+\.\d+\.\d+-matter$/.test(value.version) ||
        !Number.isInteger(value.station_id) || value.station_id < 0 || value.station_id > 16 ||
        typeof value.station_name !== 'string' || value.station_name.length > 40 ||
        !['wifi_connected','ready','paired','reachable'].every(key => typeof value[key] === 'boolean') ||
        typeof value.error !== 'string' || value.error.length > 64 ||
        !Number.isInteger(value.observed_raw) || value.observed_raw < -1 || value.observed_raw > 255 ||
        !(value.pairing_code === null || typeof value.pairing_code === 'string' && /^(\d{11}|\d{21})$/.test(value.pairing_code)) ||
        value.paired && value.pairing_code !== null || !value.ready && value.pairing_code !== null) return null;
    return value;
  } catch (_) { return null; }
}
