# Class `not_implemented`

> Header: `proxy.h`
> Since: 3.2.0

```cpp
class not_implemented : public std::exception;
```

A type of object to be thrown by the default implementation of [`weak_dispatch`](weak_dispatch.md).

## Member Functions

| Name          | Description                           |
| ------------- | ------------------------------------- |
| (constructor) | constructs a `not_implemented` object |
| (destructor)  | destroys a `not_implemented` object   |
| `what`        | returns the explanatory string        |
