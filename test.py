import phrase_tokenizer

sketch_file = open('tokens.sketch', 'r')
tokenizer = phrase_tokenizer.PhraseTokenizer(sketch_file, 7)
print tokenizer.chunk('Windows 7 SP1 64-Bit Windows 8 64-Bit Windows 8.1 64-Bit Windows 10')
