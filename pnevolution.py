import matplotlib.pyplot as plot

pn = [x.split() for x in open("pnevolution.txt", "rt").readlines()]
fig, (ax1, ax2) = plot.subplots(2)
ax1.plot([int(x[0]) for x in pn], label="proof")
ax1.plot([int(x[1]) for x in pn], label="disproof")
ax1.set(xlabel="Nodes visited", ylabel="Proof number")
ax1.legend()
ax2.plot([int(x[2]) for x in pn], label="depth")
ax2.set(xlabel="Nodes visited", ylabel="Depth")
plot.show()
