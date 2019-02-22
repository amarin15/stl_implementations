# STL implementations

This repo was created with an educational purpose in mind, so use at your own risk. That being said, the header-only implementations cover most of their STL interfaces and their APIs have been fully tested using gtest. They avoid complexities such as allocators or hints so that they're easier to follow (and totally not because I was too lazy to add them).

Currently implemented:
- `shared_ptr`
- `unique_ptr`
- `tuple`
- `unordered_map` using chaining
- `unordered_map` using open addressing with quadratic probing

Future plans:
- I would probably ignore them forever anyway, so no future plans. If I implement anything new, I'll just add it to the list above.

