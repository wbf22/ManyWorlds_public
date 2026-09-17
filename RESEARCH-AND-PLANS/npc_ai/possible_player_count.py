
import math

# --- Constants (edit these to try different scenarios) ---
bytes_per_player_update = 40      # bytes per position/rotation update
num_close_players = 40            # size of the "close" tier (fast updates)
close_update_hz = 10              # update rate for close tier (e.g. 10 Hz = every 100ms)
far_update_hz = .25                 # update rate for everyone else Hz
server_mb_per_s = 125_000_000     # server bandwidth budget in bytes/sec (e.g. 1 Gbps ≈ 125,000,000 B/s)

# --- Solve the quadratic ---
a = bytes_per_player_update * far_update_hz
b_coef = (num_close_players * bytes_per_player_update * close_update_hz - bytes_per_player_update * far_update_hz * (1 + num_close_players))
c_coef = -server_mb_per_s

discriminant = b_coef**2 - 4 * a * c_coef
n = (-b_coef + math.sqrt(discriminant)) / (2 * a)

print(f"a = {a}")
print(f"b_coef = {b_coef}")
print(f"c_coef = {c_coef}")
print(f"discriminant = {discriminant}")
print(f"Max concurrent players (n) = {n:.2f}")

# Sanity check: total bandwidth used at this n
total_bandwidth = n * (
    num_close_players * bytes_per_player_update * close_update_hz
    + (n - 1 - num_close_players) * bytes_per_player_update * far_update_hz
)
print(f"\nSanity check -- total bandwidth at n players: {total_bandwidth:,.0f} bytes/s")
print(f"Server budget: {server_mb_per_s:,} bytes/s")