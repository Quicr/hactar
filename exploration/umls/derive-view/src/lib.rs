use darling::FromDeriveInput;
use proc_macro::{self, TokenStream};
use quote::{format_ident, quote};
use syn::{parse_macro_input, DeriveInput};

#[proc_macro_derive(MyTrait, attributes(my_trait))]
pub fn derive(input: TokenStream) -> TokenStream {
    let input = parse_macro_input!(input);

    println!("input: {:?}", input);

    let DeriveInput { ident, .. } = input;

    let new_struct = format_ident!("{}View", ident);

    let output = quote! {
        struct #new_struct<'a>(&'a [u8]);

        impl<'a> core::ops::Deref for #new_struct<'a> {
            type Target = [u8];

            fn deref(&self) -> &Self::Target {
                self.0
            }
        }
    };
    output.into()
}
