use rand::Rng;

pub fn hello() -> String {
    let mut rng = rand::thread_rng();
    let number = rng.gen_range(1..=100);
    format!("Hello {}!", number)
}
