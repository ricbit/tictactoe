from collections import Counter
import reader

def collect_bits(node, level, total, children):
  total[level] += 1
  children[level] += len(node.children)
  for pos, child in node.children.items():
    collect_bits(child, level + 1, total, children)

def main():
  tree = reader.read_file("solution.txt")
  total, children = Counter(), Counter()
  collect_bits(tree.root, 0, total, children)
  maxlevel = max(total.keys())
  for level in range(maxlevel + 1):  
    print("level %d : %.2f" % (level, children[level] / total[level]))

if __name__ == '__main__':
  main()

