for i in ./*.cc; do
  echo $i
  g++ -std=c++1y -O3 -I .. $i -o ./a.out
  ./a.out
done
