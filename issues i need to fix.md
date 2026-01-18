Integer division bug
70 / 80 returning 0 means your / operator fast-path is doing int ÷ int → int, breaking Python semantics. / must always promote to floating.

Graph path / node data mismatch bug
get_shortest_path(0, 3) returned path [0, 3], but node index 3 printed Boston instead of DC, meaning your path indices and node-data lookup are out of sync or the path reconstruction is wrong.