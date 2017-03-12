import phrase_tokenizer

sketch_file = open('/Users/bob/code/computers.hella.cheap/data/tokens.sketch', 'r')
tokenizer = phrase_tokenizer.PhraseTokenizer(sketch_file, 7)
print tokenizer.chunk('Windows 7 SP1 64-Bit Windows 8 64-Bit Windows 8.1 64-Bit Windows 10',
        blacklist=set(['64-Bit']),
        whitelist=set(['Windows 7 SP1 64-Bit']))
