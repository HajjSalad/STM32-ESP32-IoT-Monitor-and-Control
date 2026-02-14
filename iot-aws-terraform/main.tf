provider "aws" {
  region = "us-east-1"
}

resource "aws_iot_thing" "esp32" {
  name = "esp32_device_1"
}

resource "aws_iot_policy" "esp32_policy" {
  name = "esp32_policy"
  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Action = ["iot:Connect", "iot:Publish", "iot:Subscribe", "iot:Receive"]
        Effect = "Allow"
        Resource = "*"
      }
    ]
  })
}

resource "aws_iot_certificate" "cert" {
  active = true
}

resource "aws_iot_policy_attachment" "policy_attach" {
  policy = aws_iot_policy.esp32_policy.name
  target = aws_iot_certificate.cert.arn
}
