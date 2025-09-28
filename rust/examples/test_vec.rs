use std::str::FromStr;

use emtrace::{expect, init, traceln};

fn main() {
    expect!(
        "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n\
         [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n\
         1f, 1f, 1f, 1f, 1f, 1f, 1f, 1f\n\
         ['A', 'vector', 'of', '&str']\n\
         A vector of &str\n\
         ['An', 'array', 'of', '&str']\n\
         ['A', 'vector', 'of', 'String']\n\
         ['A', 'vector', 'of', 'String']\n\
         ['An', 'array', 'of', 'String']\n\
         11 11 11 11\n\
         11 11 11 11\n\
         11 11 11 11\n\
         11 11 11 11\n\
         [1, 2, 3, 4, 5, 6][3, 3, 3, 3]\n\
         "
    );

    init(&mut std::io::stdout().lock()).unwrap();
    let v = vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    traceln!("{}\n{}", Vec<i32>: v, [i32]: v);

    let arr = [31; 8];
    traceln!("{:, *x}", [u16; 8]: arr);

    let v = vec!["A", "vector", "of", "&str"];
    traceln!("{}\n{: *}", Vec<&str>: v, [&str]: v);

    let arr = ["An", "array", "of", "&str"];
    traceln!("{}", [&str; 4]: arr);

    let v = vec!["A", "vector", "of", "String"]
        .iter()
        .map(|s| String::from_str(s).unwrap())
        .collect();

    traceln!("{}\n{}", Vec<String>: v, [String]: v);

    let arr = [
        String::from_str("An").unwrap(),
        String::from_str("array").unwrap(),
        String::from_str("of").unwrap(),
        String::from_str("String").unwrap(),
    ];
    traceln!("{}", [String; 4]: arr);

    let nested_static = [[3u16; 4]; 4];
    traceln!("{:\n* *b}", [[u16; 4]; 4]: nested_static);

    let nested_dynamic = [&vec![1, 2, 3, 4, 5, 6][..], &nested_static[0][..]];
    traceln!("{:*}", [&[u16]]: nested_dynamic);
}
