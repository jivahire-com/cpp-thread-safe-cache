# Extract the input file and clang-tidy args from the arguments and pass it to clang-tidy (omit the compiler invocation)
$compilerOptionsIndex = $args.IndexOf('--') - 1
$inputArgs = $args[0..$compilerOptionsIndex]
$inputFile = $args[$compilerOptionsIndex]

# If $inputFile is a path under .externs/ then don't run clang-tidy
if ($inputFile -like "$env:REPO_APP_EXTERNS_DIR/*") {
    exit 0
}

& "${env:REPO_APP_PATH_llvm.win64}/bin/clang-tidy.exe" $inputArgs $inputFile
exit $LASTEXITCODE