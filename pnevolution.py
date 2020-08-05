import matplotlib.pyplot as plot
import pandas as pd

pn = [x.split() for x in open("pnevolution.txt", "rt").readlines()]
fig, (ax1, ax2) = plot.subplots(2)
proof = [int(x[0]) for x in pn]
disproof = [int(x[1]) for x in pn]
pproof = pd.DataFrame(proof)
pdisproof = pd.DataFrame(disproof)
ax1.plot(pproof.rolling(20).mean(), label="proof")
ax1.plot(pdisproof.rolling(20).mean(), label="disproof")
ax1.set(xlabel="Nodes visited", ylabel="Proof number")
ax1.legend()
depth = [float(x[2]) for x in pn]
m = 1000
smooth = [sum(depth[i:i+m])/m for i in range(len(depth)-m)]
upper_bound = [max(smooth[i:i+m]) for i in range(len(smooth)-m)]
lower_bound = [min(smooth[i:i+m]) for i in range(len(smooth)-m)]
#ax2.plot(depth, label="depth")
ax2.plot(upper_bound, label="upper bound")
ax2.plot(lower_bound, label="lower bound")
ax2.set(xlabel="Nodes visited", ylabel="Depth")
ax2.legend()
plot.show()
