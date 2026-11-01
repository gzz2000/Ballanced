addToLibrary({
  $bwHasModSymbol__deps: ['$resolveGlobalSymbol'],
  $bwHasModSymbol: (name) => resolveGlobalSymbol(name, true).sym !== undefined,
  $bwModMetadata__deps: ['$getDylinkMetadata'],
  $bwModMetadata: (module) => getDylinkMetadata(module),
  $bwFinalizeMods__deps: ['$reportUndefinedSymbols'],
  $bwFinalizeMods: () => reportUndefinedSymbols(),
  $bwModFunction__deps: ['$getWasmTableEntry'],
  $bwModFunction: (index) => getWasmTableEntry(index),
});
