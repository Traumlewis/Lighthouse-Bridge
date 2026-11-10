// USB-only information protocol. Keys stay in page memory, never storage or URLs.
export function parseDeviceInfo(line) {
  if (!line.startsWith('LH_INFO ')) return null;
  try {
    const value = JSON.parse(line.slice(8));
    if (value.protocol !== 1 || !/^[a-f0-9]{24}$/.test(value.api_key) ||
        typeof value.version !== 'string' || value.version.length > 24 ||
        typeof value.setup_ssid !== 'string' || value.setup_ssid.length > 64 ||
        typeof value.wifi_connected !== 'boolean') return null;
    if (value.url !== null) {
      const url = new URL(value.url);
      const ip = url.hostname.split('.');
      if (url.protocol !== 'http:' || url.username || url.password || url.port ||
          url.pathname !== '/' || url.search || url.hash ||
          !(ip.length === 4 && ip.every(part => /^\d{1,3}$/.test(part) && Number(part) <= 255))) return null;
    }
    if (value.wifi_connected !== Boolean(value.url)) return null;
    return value;
  } catch (_) { return null; }
}

export async function watchDeviceInfo(port, onInfo, onEnd) {
  await port.open({baudRate:115200,bufferSize:8192});
  const reader = port.readable.getReader(), writer = port.writable.getWriter();
  let stopped = false, buffer = '', interval;
  const query = () => writer.write(new TextEncoder().encode('LH_INFO\n')).catch(() => {});
  const finished = (async () => {
    const decoder = new TextDecoder();
    try {
      await query();
      interval = setInterval(query,2000);
      while (!stopped) {
        const {value,done} = await reader.read(); if (done) break;
        buffer += decoder.decode(value,{stream:true});
        let position;
        while ((position = buffer.indexOf('\n')) >= 0) {
          const line = buffer.slice(0,position).replace(/\r$/, ''); buffer = buffer.slice(position+1);
          if (line.length <= 4096) { const info = parseDeviceInfo(line); if (info) onInfo(info); }
        }
        if (buffer.length > 4096) buffer = '';
      }
    } catch (_) { /* Disconnects end the watcher without exposing serial content. */ }
    finally {
      clearInterval(interval); reader.releaseLock(); writer.releaseLock();
      try { await port.close(); } catch (_) {}
      onEnd();
    }
  })();
  return {stop:async()=>{stopped=true;await reader.cancel().catch(()=>{});await finished;}};
}
