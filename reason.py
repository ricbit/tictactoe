from collections import Counter
import reader

hist = Counter()

def read_reason():
  reason = []
  with open("reason.hh", "rt") as f:
    for line in f:
      reason.append(line.strip().strip(","))
  return reason

def collect_bits(node):
  global hist
  hist[node.reason] += 1
  for pos, child in node.children.items():
    collect_bits(child)

def main():
  tree = reader.read_file("solution.txt")
  reason = read_reason()
  collect_bits(tree.root)
  for k, v in hist.items():
    print(reason[k], v)

if __name__ == '__main__':
  main()

