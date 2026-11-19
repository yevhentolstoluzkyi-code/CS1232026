use std::process::ExitCode;

fn contains_substring(line: &str, pattern: &str) -> bool {
    line.contains(pattern)
}

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
    for (idx, line) in content.lines().enumerate() {
        if contains_substring(line, &args[1]) {
            println!("{}: {}", idx + 1, line);
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
    use super::*;

    #[test]
    fn finds_substring() {
        assert!(contains_substring("рядок з malloc", "malloc"));
        assert!(!contains_substring("рядок без збігу", "malloc"));
    }
}
