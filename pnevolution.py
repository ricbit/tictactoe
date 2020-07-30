import matplotlib.pyplot as plot

pn = [x.split() for x in open("pnevolution.txt", "rt").readlines()]
fig, (ax1, ax2) = plot.subplots(2)
ax1.plot([int(x[0]) for x in pn], label="proof")
ax1.plot([int(x[1]) for x in pn], label="disproof")
ax1.set(xlabel="Nodes visited", ylabel="Proof number")
ax1.legend()
depth = [float(x[2]) for x in pn]
m = 100
smooth = [sum(depth[i:i+m])/m for i in range(len(depth)-m)]
upper_bound = [max(smooth[i:i+m]) for i in range(len(smooth)-m)]
lower_bound = [min(smooth[i:i+m]) for i in range(len(smooth)-m)]
#ax2.plot(depth, label="depth")
ax2.plot(upper_bound, label="upper bound")
ax2.plot(lower_bound, label="lower bound")
ax2.set(xlabel="Nodes visited", ylabel="Depth")
ax2.legend()
plot.show()
