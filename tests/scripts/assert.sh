DOJO="./build/dojo"
assert() {
  expected="$1"
  input="$2"
  
  echo -e "$input" > test.dojo


  actual=`$DOJO test.dojo`

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    printRedText "$input => $expected expected, but got $actual"
    exit 1
  fi
}

printRedText() {
  RED='\033[0;31m'
  NC='\033[0m'
  echo -e "${RED}$1${NC}"
}

printGreenText() {
  GREEN='\033[0;32m'
  NC='\033[0m'
  echo -e "${GREEN}$1${NC}"
}

suite() {
  echo "[SUITE] $1"
}


cleanup() {
  rm -f test.dojo
}