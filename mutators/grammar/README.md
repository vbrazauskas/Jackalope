# Grammar-based mutation

Jackalope supports sample generation and subsequent mutation based on a simple grammar format, similar to that used by [Domato](https://github.com/googleprojectzero/domato).

To use grammar-based generation/mutation, simply add `-grammar <path/to/grammar/file>` to the Jackalope command line.

The grammar file is parsed line by line, where each line contains one grammar rule. Everything after a `#` character is considered a comment. A grammar rule should be given in the following form:

```
<symbol> = a mix of constants and <other_symbol>s
```

Each grammar rule contains a left side and the right side separated by the equal character. The left side contains a symbol, while the right side contains the details on how that symbol may be expanded. When expanding a symbol, all symbols on the right-hand side are expanded recursively while everything that is not a symbol is simply copied to the output. Note that a single rule can't span over multiple lines of the input file.

An example of such a grammar for JavaScript can be found in `examples/grammar/jsgrammar.txt`

Domato-style symbol attributes are currently not supported. The following special symbols are supported:

- `<lt>` - ‘<’ character
- `<gt>` - ‘>’ character
- `<hash>` - ‘#’ character
- `<cr>` - CR character
- `<lf>` - LF character
- `<space>` - space character
- `<tab>` - tab character
- Any symbol whose name starts with `0x` is interpreted as a hex-encoded string. For example, `<0x414141>` will output the string `AAA`

In addition to these, any symbol whose name starts with `repeat_` has a special meaning: It will output zero or more symbols whose name is specified after repeat. In the example JavaScript grammar, the symbol `<repeat_statement>` will output zero or more `<statement>`s. `<repeat_>` symbols give a hint to the mutation and minimization engines how these symbols can be modified while maintaining the validity of the sample.

When a sample is generated by a grammar, the sample is represented in a tree form which allows easy manipulation for e.g. mutation and minimization operations. When such a sample is saved to a file, it gets convered to a binary representation that can be decoded back into the tree form.

## Mutation

Grammar-based mutation is implemented in `grammarmutator.cpp`

In addition to generating a new sample based on the grammar, the following mutation operations can currently be performed on a previously generated sample:

 - Selects a random node in the sample's tree representation. Generate this node anew (naturally, this also affects the selected node's children nodes) while keeping the remaining nodes unchanged.
 - Splice: Selects a random node from the current sample and a node with the same symbol from some other sample. Replace the node in the current sample with a node from the other sample.
 - Repeat node mutation: One or more new children get added to a `<repeat_>` node, or replace some of the existing children node.
 - Repeat splice: Selects a `<repeat_>` node from the current sample and a similar `<repeat_>` node from another sample. Mixes children from the other node into the current node.

## Minimization

Grammar-aware minimization is implemented in `grammarminimizer.cpp`

A minimization is performed to reduce the size of samples in the fuzzer corpus. When a sample is found that hits new coverage in the target program, the minimizer attempts reduce its size to produce a smaller sample that exercises the same new coverage. Crash minimization is currently not performed.

The minimizer attempts to iteratively remove nodes from the sample, while still keeping the sample valid in the context of the currently used grammar. The following nodes can be safely removed:

 - Any symbol that can generate an empty string can have all of its children nodes removed. In the following example, `<some_symbol>` can generate `foo<bar>` but it can also generate an empty string. Any node generated by the first rule can be emptied.

```
<some_symbol> = foo<bar>
<some_symbol> = 
```

 - A node representing a `<repeat_>` symbol could have one or more (or potentially all) of its child nodes removed. In the JavaScript example, the `<repeat_statement>` symbol tells the minimizer that some of the JavaScript statements could be removed from the `<repeat_statement>` nodes while maintaining the validity of the sample.