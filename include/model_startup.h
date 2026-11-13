#pragma once
namespace bridge {
// The backend owns the recovery reserve. Do not publish/enable a partial model;
// release reserve/partial objects before reporting failure to the web interface.
template<class Backend> bool buildMatterModel(Backend& backend) {
  const bool ready = backend.construct() && backend.healthy() && backend.activate() && backend.healthy();
  backend.release();
  if (!ready) { backend.rollback(); backend.failed(); }
  return ready;
}
} // namespace bridge
