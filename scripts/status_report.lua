-- status_report.lua
-- Counts HTTP status codes seen by wrk, e.g. 200, 404, 500.

threads = {}

function setup(thread)
  table.insert(threads, thread)
end

function response(status, headers, body)
  local key = "status_" .. tostring(status)
  _G[key] = (_G[key] or 0) + 1
end

function done(summary, latency, requests)
  print("")
  print("=== WRK SUMMARY ===")
  print(string.format("requests_completed: %d", summary.requests))
  print(string.format("bytes: %d", summary.bytes))
  print(string.format("duration_sec: %.3f", summary.duration / 1000000.0))

  print("")
  print("=== SOCKET / WRK ERRORS ===")
  print(string.format("connect_errors: %d", summary.errors.connect))
  print(string.format("read_errors: %d", summary.errors.read))
  print(string.format("write_errors: %d", summary.errors.write))
  print(string.format("timeout_errors: %d", summary.errors.timeout))
  print(string.format("non_2xx_3xx_responses: %d", summary.errors.status))

  print("")
  print("=== HTTP STATUS COUNTS ===")

  local total_status = 0

  for code = 100, 599 do
    local name = "status_" .. tostring(code)
    local count = 0

    for _, thread in ipairs(threads) do
      count = count + (thread:get(name) or 0)
    end

    if count > 0 then
      print(string.format("%d: %d", code, count))
      total_status = total_status + count
    end
  end

  print("")
  print(string.format("responses_seen_by_lua: %d", total_status))
end