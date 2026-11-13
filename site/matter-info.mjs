// Experimental protocol: pairing codes stay in page memory and on physical USB.
export function parseMatterInfo(line) {
  if (!line.startsWith('LH_MATTER_INFO ')) return null;
  try {
    const value = JSON.parse(line.slice(15));
    if (value.protocol !== 1 || !/^\d+\.\d+\.\d+-(?:matter|tasmota)$/.test(value.version) ||
        !Number.isInteger(value.station_id) || value.station_id < 0 || value.station_id > 16 ||
        typeof value.station_name !== 'string' || value.station_name.length > 40 ||
        !['wifi_connected','ready','paired','reachable'].every(key => typeof value[key] === 'boolean') ||
        typeof value.error !== 'string' || value.error.length > 64 ||
        !Number.isInteger(value.observed_raw) || value.observed_raw < -1 || value.observed_raw > 255 ||
        !(value.pairing_code === null || typeof value.pairing_code === 'string' && /^(\d{11}|\d{21})$/.test(value.pairing_code)) ||
        value.paired && value.pairing_code !== null || !value.ready && value.pairing_code !== null ||
        value.bridge_name !== undefined && (typeof value.bridge_name !== 'string' || value.bridge_name.length > 32)) return null;
    if (value.stations !== undefined) {
      if (!Array.isArray(value.stations) || value.stations.length > 16) return null;
      const seen = new Set();
      for (const station of value.stations) {
        if (!Number.isInteger(station.id) || station.id < 1 || station.id > 16 || seen.has(station.id) ||
            typeof station.name !== 'string' || station.name.length > 40 || typeof station.reachable !== 'boolean' ||
            !Number.isInteger(station.observed_raw) || station.observed_raw < -1 || station.observed_raw > 255) return null;
        seen.add(station.id);
      }
    }
    return value;
  } catch (_) { return null; }
}
