import matplotlib.pyplot as plot

pn = [x.split() for x in open("pnevolution.txt", "rt").readlines()]
#plot.plot([int(x[0]) for x in pn], label="proof")
#plot.plot([int(x[1]) for x in pn], label="disproof")
plot.plot([int(x[2]) for x in pn], label="depth")
plot.xlabel("Nodes visited")
plot.ylabel("Proof number")
plot.legend()
plot.show()
