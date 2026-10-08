# Named requirements: *ProBasicMetadata*

> Since: 5.0.0

A type `M` meets the *ProBasicMetadata* requirements if `M` is a class type, and the following expressions are well-formed, non-throwing, and have the specified semantics (let `m` be an object of type `M`, and `cm` be a reference to `const M`).

| Expressions | Semantics                                                   |
| ----------- | ----------------------------------------------------------- |
| `M(cm)`     | Creates an object of type `M` holding the metadata of `cm`. |
| `m.~M()`    | Destroys the object `m`.                                    |

## Notes

"Metadata" is an object holding information deduced from a type at compile time, stored in or referenced by a [`proxy`](proxy/README.md). Because `proxy` copies and destroys metadata in contexts specified not to throw, each expression above must be non-throwing, as determined by the `noexcept` operator. A function that never throws at runtime but has a potentially throwing exception specification does not satisfy this requirement.

## See Also

- [*ProBasicReflection* requirements](ProBasicReflection.md)
- [*ProMetadata* requirements](ProMetadata.md)
- [*ProMetadataPolicy* requirements](ProMetadataPolicy.md)
