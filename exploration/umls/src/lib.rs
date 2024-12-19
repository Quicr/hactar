#![allow(dead_code)]

use derive_view::MyTrait;

trait MyTrait {
    const RESULT: usize;
}

struct Bar;
struct Baz;

#[derive(MyTrait)]
struct Foo {
    bar: Bar,
    baz: Baz,
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn it_works() {
        println!("view: {:?}", FooView(&[1, 2, 3]).len());
    }
}

/*
impl Foo {
    fn to_bytes(data: &mut [u8]) -> Result<()>;
}

struct FooView<'a> {
    bar: BarView<'a>,
    baz: BazView<'a>,
}

impl FooView<'a> {
    fn from_bytes(data: &'a [u8]) -> Result<Self>;
    fn to_owned(self) -> Foo;
}
*/

/*
mod common {
    use core::convert::AsRef;
    use core::fmt::Debug;
    use core::ops::Deref;

    #[derive(Debug, PartialEq)]
    pub struct Error;

    pub type Result<T> = core::result::Result<T, Error>;

    pub trait Encode<'a>: Sized + PartialEq + Debug {
        const MAX_SIZE: usize;
        type Contents;

        fn build(contents: Self::Contents, data: &mut [u8]) -> Result<usize>;
        fn parse(data: &'a [u8]) -> Result<(Self, usize)>;
    }

    #[derive(Copy, Clone, PartialEq, Debug)]
    pub struct Varint(usize);

    impl<'a> Encode<'a> for Varint {
        const MAX_SIZE: usize = 4;
        type Contents = usize;

        fn build(val: usize, data: &mut [u8]) -> Result<usize> {
            if val < (1 << 6) {
                data[0] = val as u8;
                Ok(1)
            } else if val < (1 << 14) {
                data[0] = ((val >> 8) as u8) | 0x40;
                data[1] = val as u8;
                Ok(2)
            } else if val < (1 << 30) {
                data[0] = ((val >> 24) as u8) | 0x80;
                data[1] = (val >> 16) as u8;
                data[2] = (val >> 8) as u8;
                data[3] = val as u8;
                Ok(4)
            } else {
                Err(Error)
            }
        }

        fn parse(data: &'a [u8]) -> Result<(Self, usize)> {
            let length = 1 << usize::from(data[0] >> 6);

            let val = match length {
                1 => usize::from(data[0]) & 0x3f,
                2 => {
                    let val = usize::from(data[0]) & 0x3f;
                    let val = (val << 8) + usize::from(data[1]);
                    val
                }
                4 => {
                    let val = usize::from(data[0]) & 0x3f;
                    let val = (val << 8) + usize::from(data[1]);
                    let val = (val << 8) + usize::from(data[2]);
                    let val = (val << 8) + usize::from(data[3]);
                    val
                }
                _ => return Err(Error),
            };

            Ok((Self(val), length))
        }
    }

    const fn varint_size(n: usize) -> usize {
        if n < (1 << 6) {
            1
        } else if n < (1 << 14) {
            2
        } else if n < (1 << 30) {
            4
        } else {
            unreachable!();
        }
    }

    #[derive(PartialEq, Debug)]
    pub struct BoundedOpaque<'a, const N: usize>(&'a [u8]);

    impl<'a, const N: usize> BoundedOpaque<'a, N> {
        pub fn new(data: &'a [u8]) -> Result<Self> {
            if data.len() <= N {
                Ok(Self(data))
            } else {
                Err(Error)
            }
        }
    }

    impl<'a, const N: usize> Deref for BoundedOpaque<'a, N> {
        type Target = [u8];

        fn deref(&self) -> &Self::Target {
            &self.0
        }
    }

    impl<'a, const N: usize> Encode<'a> for BoundedOpaque<'a, N> {
        const MAX_SIZE: usize = varint_size(N) + N;
        type Contents = &'a [u8];

        fn build(contents: &[u8], data: &mut [u8]) -> Result<usize> {
            let n = contents.len();
            if n > N {
                return Err(Error);
            }

            let start = Varint::build(n, data)?;
            let end = start + n;
            data[start..end].copy_from_slice(contents);
            Ok(end)
        }

        fn parse(data: &'a [u8]) -> Result<(Self, usize)> {
            let (Varint(n), start) = Varint::parse(data)?;
            if n > N {
                return Err(Error);
            }

            let end = start + n;
            let contents = &data[start..end];
            Ok((Self(contents), end))
        }
    }

    #[derive(PartialEq, Debug)]
    pub struct FixedOpaque<'a, const N: usize>(&'a [u8]);

    impl<'a, const N: usize> FixedOpaque<'a, N> {
        pub fn new(data: &'a [u8]) -> Result<Self> {
            if data.len() == N {
                Ok(Self(data))
            } else {
                Err(Error)
            }
        }
    }

    impl<'a, const N: usize> Deref for FixedOpaque<'a, N> {
        type Target = [u8];

        fn deref(&self) -> &Self::Target {
            &self.0
        }
    }

    impl<'a, const N: usize> Encode<'a> for FixedOpaque<'a, N> {
        const MAX_SIZE: usize = varint_size(N) + N;
        type Contents = &'a [u8];

        fn build(contents: &[u8], data: &mut [u8]) -> Result<usize> {
            let n = contents.len();
            if n != N {
                return Err(Error);
            }

            let start = Varint::build(n, data)?;
            let end = start + n;
            data[start..end].copy_from_slice(contents);
            Ok(end)
        }

        fn parse(data: &'a [u8]) -> Result<(Self, usize)> {
            let (Varint(n), start) = Varint::parse(data)?;
            if n != N {
                return Err(Error);
            }

            let end = start + n;
            let contents = &data[start..end];
            Ok((Self(contents), end))
        }
    }

    pub trait FixedValue: Default {
        type ValueType: AsRef<[u8]>;
        const FIXED_VALUE: Self::ValueType;
        const FIXED_LEN: usize;
    }

    impl<'a, T> Encode<'a> for T
    where
        T: FixedValue + Sized + PartialEq + Debug,
    {
        const MAX_SIZE: usize = Self::FIXED_LEN;
        type Contents = ();

        fn build(_contents: (), data: &mut [u8]) -> Result<usize> {
            let end = Self::FIXED_VALUE.as_ref().len();
            data[..end].copy_from_slice(Self::FIXED_VALUE.as_ref());
            Ok(end)
        }

        fn parse(data: &'a [u8]) -> Result<(Self, usize)> {
            let end = Self::FIXED_VALUE.as_ref().len();
            if &data[..end] != Self::FIXED_VALUE.as_ref() {
                return Err(Error);
            }

            Ok((Self::default(), end))
        }
    }

    #[cfg(test)]
    mod test {
        use super::*;

        fn encode_test<'a, T: Encode<'a>>(
            val: T::Contents,
            data: &'a mut [u8],
            expected_enc: &[u8],
            expected_dec: T,
        ) {
            let len = T::build(val, data).unwrap();
            assert_eq!(len, data.len());
            assert_eq!(data, expected_enc);

            let (parsed, len) = T::parse(data).unwrap();
            assert_eq!(len, data.len());
            assert_eq!(parsed, expected_dec);
        }

        #[test]
        fn varint() {
            encode_test::<Varint>(0x3f, &mut [0u8; 1], &[0x3f], Varint(0x3f));
            encode_test::<Varint>(0x3fff, &mut [0u8; 2], &[0x7f, 0xff], Varint(0x3fff));
            encode_test::<Varint>(
                0x3fffffff,
                &mut [0u8; 4],
                &[0xbf, 0xff, 0xff, 0xff],
                Varint(0x3fffffff),
            );
        }

        #[test]
        fn bounded_opaque() {
            const SIZE: usize = 63;
            type Opaque<'a> = BoundedOpaque<'a, SIZE>;

            const BUFFER_SIZE: usize = Opaque::MAX_SIZE;

            let buffer = &mut [0u8; BUFFER_SIZE];

            let contents = &[0xa0; SIZE];
            let expected_dec = BoundedOpaque::<SIZE>(contents);

            let expected_enc = &{
                let mut expected = [0xa0; BUFFER_SIZE];
                expected[0] = SIZE as u8;
                expected
            };

            encode_test::<Opaque>(contents, buffer, expected_enc, expected_dec);

            // Test that invalid size fails
            let mut too_big = [0; SIZE + 2];
            too_big[0..2].copy_from_slice(&[0x7f, 0xff]);
            assert_eq!(Opaque::build(&too_big, &mut [0; 2]), Err(Error));
            assert_eq!(Opaque::parse(&too_big), Err(Error));
        }

        #[test]
        fn fixed_opaque() {
            const SIZE: usize = 63;
            type Opaque<'a> = FixedOpaque<'a, SIZE>;

            const BUFFER_SIZE: usize = Opaque::MAX_SIZE;

            let buffer = &mut [0u8; BUFFER_SIZE];

            let contents = &[0xa0; SIZE];
            let expected_dec = FixedOpaque::<SIZE>(contents);

            let expected_enc = &{
                let mut expected = [0xa0; BUFFER_SIZE];
                expected[0] = SIZE as u8;
                expected
            };

            encode_test::<Opaque>(contents, buffer, expected_enc, expected_dec);

            // Test that invalid size fails
            let mut too_big = [0; SIZE + 2];
            too_big[0..2].copy_from_slice(&[0x7f, 0xff]);
            assert_eq!(Opaque::build(&too_big, &mut [0; 2]), Err(Error));
            assert_eq!(Opaque::parse(&too_big), Err(Error));

            let mut too_small = [0; SIZE];
            too_small[0] = SIZE - 1;
            assert_eq!(Opaque::build(&too_big, &mut [0; 2]), Err(Error));
            assert_eq!(Opaque::parse(&too_big), Err(Error));
        }
    }
}

mod crypto {
    use crate::common::FixedOpaque;

    // XXX(RLB) In an ideal world, these constanst would be generics, so that they could be supplied by
    // the application at build time.  However, Rust's support for const generics is not complete
    // enough to support this without a bunch of hassle.  (The insanity that the `ml_kem` and `ml_dsa`
    // crates undertook [1] would be even crazier for a full protocol.) So instead we define batches
    // of constants that can be selected with feature flags, so that the application still has a degree
    // of choice, but with less hassle from dealing with generics.
    //
    // [1] https://github.com/RustCrypto/KEMs/blob/master/ml-kem/src/param.rs#L196
    //
    // Note that enabling more than one of these features will result in duplicate symbols.
    #[cfg(feature = "x25519_aes128gcm_ed25519")]
    pub mod consts {
        pub const CIPHER_SUITE: [u8; 2] = [0x00, 0x01];

        pub const HASH_OUTPUT_SIZE: usize = 32;

        pub const HPKE_PUBLIC_KEY_SIZE: usize = 32;

        pub const SIGNATURE_PUBLIC_KEY_SIZE: usize = 32;
        pub const SIGNATURE_SIZE: usize = 64;
    }

    pub type HashOutput<'a> = FixedOpaque<'a, { consts::HASH_OUTPUT_SIZE }>;

    pub type HpkePublicKey<'a> = FixedOpaque<'a, { consts::HPKE_PUBLIC_KEY_SIZE }>;

    pub type SignaturePublicKey<'a> = FixedOpaque<'a, { consts::SIGNATURE_PUBLIC_KEY_SIZE }>;

    pub type Signature<'a> = FixedOpaque<'a, { consts::SIGNATURE_SIZE }>;
}

mod protocol {
    use crate::common::*;
    use crate::crypto::*;

    use hex_literal::hex;

    const fn max(a: usize, b: usize) -> usize {
        if a < b {
            a
        } else {
            b
        }
    }

    // XXX(RLB) Similar story here to the cryptographic parameters, except here the need for
    // application modification is even more acute.  We ought to define some options here, with feature
    // flags to select among them.
    mod consts {
        pub const MAX_CREDENTIAL_SIZE: usize = 128;
    }

    pub type Credential<'a> = BoundedOpaque<'a, { consts::MAX_CREDENTIAL_SIZE }>;

    #[derive(Default, PartialEq, Debug)]
    pub struct Capabilities;

    impl FixedValue for Capabilities {
        type ValueType = [u8; 11];

        // versions      = 02 0001  // MLS 1.0
        // cipher_suites = 02 xxxx  // The one fixed cipher suite
        // extensions    = 00       // No extensions
        // proposals     = 00       // No proposals
        // credentials   = 02 0001  // Basic credentials
        const FIXED_VALUE: [u8; 11] = {
            let mut value = hex!("02 0001 02 0000 00 00 02 0001");
            value[4] = crate::crypto::consts::CIPHER_SUITE[0];
            value[5] = crate::crypto::consts::CIPHER_SUITE[1];
            value
        };

        const FIXED_LEN: usize = 11;
    }

    #[derive(Default, PartialEq, Debug)]
    pub struct LeafNodeExtensions;

    impl FixedValue for LeafNodeExtensions {
        type ValueType = [u8; 1];
        const FIXED_VALUE: [u8; 1] = [0];
        const FIXED_LEN: usize = 1;
    }

    #[derive(PartialEq, Debug)]
    pub enum LeafNodeSource<'a> {
        KeyPackage,
        Update,
        Commit(HashOutput<'a>),
    }

    impl<'a> LeafNodeSource<'a> {
        const KEY_PACKAGE: u8 = 1;
        const UPDATE: u8 = 2;
        const COMMIT: u8 = 3;
    }

    impl<'a> Encode<'a> for LeafNodeSource<'a> {
        const MAX_SIZE: usize = max(16, HashOutput::MAX_SIZE);
        type Contents = &'a Self;

        fn build(contents: &'a Self, data: &mut [u8]) -> Result<usize> {
            match contents {
                Self::KeyPackage => {
                    data[0] = Self::KEY_PACKAGE;
                    data[1..9].copy_from_slice(&0_u64.to_be_bytes());
                    data[9..17].copy_from_slice(&u64::MAX.to_be_bytes());
                    Ok(17)
                }
                Self::Update => {
                    data[0] = Self::UPDATE;
                    Ok(1)
                }
                Self::Commit(parent_hash) => {
                    data[0] = Self::COMMIT;
                    let hash_len = HashOutput::build(parent_hash, &mut data[1..])?;
                    Ok(1 + hash_len)
                }
            }
        }

        fn parse(data: &'a [u8]) -> Result<(Self, usize)> {
            let source = data[0];
            let data = &data[1..];
            match source {
                Self::KEY_PACKAGE => {
                    let mut not_before = [0u8; 8];
                    not_before.copy_from_slice(&data[0..8]);
                    let not_before = u64::from_be_bytes(not_before);

                    let mut not_after = [0u8; 8];
                    not_after.copy_from_slice(&data[8..16]);
                    let not_after = u64::from_be_bytes(not_after);

                    if not_before != 0 || not_after != u64::MAX {
                        return Err(Error);
                    }

                    Ok((Self::KeyPackage, 17))
                }
                Self::UPDATE => Ok((Self::Update, 1)),
                Self::COMMIT => {
                    let (parent_hash, hash_len) = HashOutput::parse(data)?;
                    Ok((Self::Commit(parent_hash), 1 + hash_len))
                }
                _ => Err(Error),
            }
        }
    }

    #[derive(PartialEq, Debug)]
    pub struct LeafNode<'a> {
        encryption_key: HpkePublicKey<'a>,
        signature_key: SignaturePublicKey<'a>,
        credential: Credential<'a>,
        leaf_node_source: LeafNodeSource<'a>,
        signature: Signature<'a>,
    }

    impl<'a> LeafNode<'a> {
        fn new(
            signature_key: SignaturePublicKey<'a>,
            credential: Credential<'a>,
            data: &mut [u8],
        ) -> Result<Self> {
            // Generate encryption key
            let encryption_key_storage = HpkePublicKey::make_storage();
            let encryption_key = HpkePublicKey::new(&encryption_key_storage)?;
            // TODO(RLB) CIPHER_SUITE.generate_hpke_key()
            // TODO(RLB) return private state

            // Make a temporary, blank signature
            let signature_storage = Signature::make_storage();
            let signature = Signature::new(&signature_storage)?;

            // Assemble preliminary self object
            let preliminary_leaf = LeafNode {
                encryption_key,
                signature_key,
                credential,
                leaf_node_source: LeafNodeSource::KeyPackage,
                signature,
            };

            // TODO build() / parse()
            // TODO sign()

            todo!()
        }
    }

    impl<'a> Encode<'a> for LeafNode<'a> {
        const MAX_SIZE: usize = HpkePublicKey::MAX_SIZE
            + SignaturePublicKey::MAX_SIZE
            + Credential::MAX_SIZE
            + Capabilities::MAX_SIZE
            + LeafNodeSource::MAX_SIZE
            + LeafNodeExtensions::MAX_SIZE
            + Signature::MAX_SIZE;
        type Contents = &'a Self;

        fn build(contents: &'a Self, data: &mut [u8]) -> Result<usize> {
            let mut n = 0;
            n += HpkePublicKey::build(&contents.encryption_key, &mut data[n..])?;
            n += SignaturePublicKey::build(&contents.signature_key, &mut data[n..])?;
            n += Credential::build(&contents.credential, &mut data[n..])?;
            n += Capabilities::build((), &mut data[n..])?;
            n += LeafNodeSource::build(&contents.leaf_node_source, &mut data[n..])?;
            n += LeafNodeExtensions::build((), &mut data[n..])?;
            n += Signature::build(&contents.signature, &mut data[n..])?;

            Ok(n)
        }

        fn parse(data: &'a [u8]) -> Result<(Self, usize)> {
            let mut n = 0;
            let (encryption_key, dn) = HpkePublicKey::parse(&data[n..])?;
            n += dn;

            let (signature_key, dn) = SignaturePublicKey::parse(&data[n..])?;
            n += dn;

            let (credential, dn) = Credential::parse(&data[n..])?;
            n += dn;

            let (_, dn) = Credential::parse(&data[n..])?;
            n += dn;

            let (leaf_node_source, dn) = LeafNodeSource::parse(&data[n..])?;
            n += dn;

            let (_, dn) = LeafNodeExtensions::parse(&data[n..])?;
            n += dn;

            let (signature, dn) = Signature::parse(&data[n..])?;
            n += dn;

            let leaf_node = LeafNode {
                encryption_key,
                signature_key,
                credential,
                leaf_node_source,
                signature,
            };

            Ok((leaf_node, n))
        }
    }
}
*/

/*
mod encode {
    use crate::common::*;
    use crate::crypto::*;
    use crate::protocol::*;

    const fn max(a: usize, b: usize) -> usize {
        if a < b {
            a
} else {
            b
        }
    }

    const fn varint_size(n: usize) -> usize {
        if n < (1 << 6) {
            1
        } else if n < (1 << 14) {
            2
        } else if n < (1 << 30) {
            4
        } else {
            unreachable!();
        }
    }

    trait Encode {
        const MAX_SIZE: usize;
    }

    impl<const N: usize> Encode for FixedOpaque<N> {
        const MAX_SIZE: usize = varint_size(N) + N;
    }

    impl<const N: usize> Encode for Opaque<N> {
        const MAX_SIZE: usize = varint_size(N) + N;
    }

    impl Encode for Capabilities {
        const MAX_SIZE: usize = Capabilities::FIXED_LEN;
    }

    impl Encode for LeafNodeSource {
        const MAX_SIZE: usize = max(16, HashOutput::MAX_SIZE);
    }

    impl Encode for LeafNodeExtensions {
        const MAX_SIZE: usize = 1;
    }

    impl Encode for LeafNode {
        const MAX_SIZE: usize = HpkePublicKey::MAX_SIZE
            + SignaturePublicKey::MAX_SIZE
            + Credential::MAX_SIZE
            + Capabilities::MAX_SIZE
            + LeafNodeSource::MAX_SIZE
            + LeafNodeExtensions::MAX_SIZE
            + Signature::MAX_SIZE;
    }
}
*/
