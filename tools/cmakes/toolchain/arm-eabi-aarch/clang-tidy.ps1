# Extract the input file and clang-tidy args from the arguments and pass it to clang-tidy (omit the compiler invocation)
$compilerOptionsIndex = $args.IndexOf('--') - 1
$inputArgs = $args[0..$compilerOptionsIndex]
$inputFile = $args[$compilerOptionsIndex]
$inputArgs = @('--quiet') +
@("--extra-arg=--target=thumb") +
@("--extra-arg=-mfloat-abi=soft") + 
@("--extra-arg=-Qunused-arguments") + 
@("--extra-arg=-Qunused-arguments") +
@("--extra-arg=-isystem") + $inputArgs +
@("--extra-arg=${env:REPO_APP_PATH_gcc.arm.eabi.aarch-win64}/arm-none-eabi/include") +
@("-p") +
@("${env:REPO_APP_TARGET_BUILD_DIR}/compile_commands.json")

# If $inputFile is a path under .externs/ then don't run clang-tidy
if ($inputFile -like "$env:REPO_APP_EXTERNS_DIR/*") {
    exit 0

}

& "${env:REPO_APP_PATH_llvm.win64}/bin/clang-tidy.exe" $inputArgs $inputFile
exit $LASTEXITCODE