addToLibrary({
  $bwFinalizeMods__deps: ['$reportUndefinedSymbols'],
  $bwFinalizeMods: () => reportUndefinedSymbols(),
  $bwModFunction__deps: ['$getWasmTableEntry'],
  $bwModFunction: (index) => getWasmTableEntry(index),
});
