## Stupid cache
I don't known why is smart cache smart. But I do know this one is not. So it must be stupid.

Stupid cache is a hash map assisted fifo with a size limit. Which is used to sotre temporary K-V paires. It is useful for F(key) = value where for a given key there is only a single value, and the key may repeat over time.
