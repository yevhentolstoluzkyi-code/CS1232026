use std::process::ExitCode;

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().collect();

    if args.len() != 3 {
        eprintln!("Використання: {} ШАБЛОН ФАЙЛ", args[0]);
        return ExitCode::from(2);
    }

    let content = match std::fs::read_to_string(&args[2]) {
        Ok(text) => text,
        Err(e) => {
            eprintln!("{}: {}", args[2], e);
            return ExitCode::from(2);
        }
    };

    let mut found = false;

    for (number, line) in content.lines().enumerate() {
        if line.contains(&args[1]) {
            println!("{}:{}", number + 1, line);
            found = true;
        }
    }

    if found {
        ExitCode::from(0)
    } else {
        ExitCode::from(1)
    }
}

#[cfg(test)]
mod tests {
    #[test]
    fn finds_substring() {
        assert!("рядок з malloc".contains("malloc"));
    }
}
