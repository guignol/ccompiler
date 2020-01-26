[低レイヤを知りたい人のためのCコンパイラ作成入門](https://www.sigbus.info/compilerbook) を読みながら適当に書いてく

# CLion on Windows with CMake on WSL

## CMake

+ CLionがMakeをサポートしてないのでCMakeを頑張る

## build

+ Windows側のプロジェクトをCLionで開く
+ CLionがWSL側のビルドツールを使って、Windows側のディレクトリに結果を出力する
    + sftpでツール呼び出ししてるっぽいので、標準出力などはたぶんsftp経由

## run & test

+ CLionのTerminalをWSLのものにしておく
+ そのターミナルで`./test.sh`を実行する
+ いずれはCLion側から実行したい

## CRLF

```
git config --global core.autocrlf false
git clone https://github.com/guignol/ccompiler.git
git config --global core.autocrlf true
```
